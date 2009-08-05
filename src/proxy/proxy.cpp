//Copyright GPLv2/3  Frederik M.J.V. 
#include "proxy.h"
#include "proxymessageprocessor.h"
#include "chatlinemodel.h"
#include "connectionmanager.h"
#include "identity.h"
#include "bufferinfo.h"
#include "backlogrequester.h"
#include "protocol.pb.h"
#include "prototools.h"
#include <QDateTime>
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#define MAX_IDLE_TIME 60
void getDiff(const google::protobuf::Message *m1,const google::protobuf::Message *m2,google::protobuf::Message *mt);
Proxy::Proxy(QTcpSocket *client,int sid){
    conn=new ConnectionManager();
    back=new BacklogRequester();
    this->client=client;
    connect(client,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    connect(client,SIGNAL(readyRead()),this,SLOT(clientHasData()));
    nextChanId=0;
    nextUserId=0;
    activeBuffer=0;
    newRead=true;
    clientPause=false;
    clientDisconnected=false;
    syncronizing=false;
    lastActivity=QDateTime::currentDateTime();
    activityChecker.setInterval(5000);
    connect(&activityChecker,SIGNAL(timeout()),this,SLOT(checkActivity()));
    activityChecker.start();
    setup.set_sessionid(sid);
}
Proxy::~Proxy(){
    activityChecker.stop();
    delete conn;
    if(client!=NULL){
        delete client;
    }
}

MessageModel *Proxy::createMessageModel(QObject *parent){
  return new ChatLineModel(parent);
}
AbstractMessageProcessor *Proxy::createMessageProcessor(QObject *parent){
  return new ProxyMessageProcessor(parent,this);
}


void Proxy::init(){
    QVariantMap map;
    map.insert("Host","lekebilen.com");
    map.insert("Port",4243);
    conn->connectToCore(map);
    //syncer = new ClientSyncer(this);
    //Client::registerClientSyncer(syncer);
    //connect(syncer, SIGNAL(connectionError(const QString &)), this, SLOT(initPhaseError(const QString &)));
    connect(conn, SIGNAL(startLogin()), this, SLOT(startLogin()));
    connect(conn, SIGNAL(connectionEstablished()), this, SLOT(connectedToCore()));
    connect(conn, SIGNAL(gotCoreSessionState(const QVariantMap&)), this, SLOT(gotCoreSessionState(const QVariantMap&)));
    connect(conn, SIGNAL(loginFailed(const QString &)), this, SLOT(loginFailed(const QString &)));
    //connect(conn, SIGNAL(syncronizeNetwork(const QVariant&)), this, SLOT(syncronizeNetwork(const QVariant&)));

  /*connect(syncer, SIGNAL(connectionWarnings(const QStringList &)), this, SLOT(initPhaseWarning(const QStringList &)));
  connect(syncer, SIGNAL(connectionMsg(const QString &)), this, SLOT(initPhaseMessage(const QString &)));
  connect(syncer, SIGNAL(startLogin()), this, SLOT(startLogin()));

  connect(syncer, SIGNAL(loginSuccess()), this, SLOT(startSync()));
  connect(syncer, SIGNAL(startCoreSetup(const QVariantList &)), this, SLOT(startCoreConfig(const QVariantList &)));*/
  //connect(syncer, SIGNAL(sessionProgress(quint32, quint32)), this, SLOT(coreSessionProgress(quint32, quint32)));
  //connect(syncer, SIGNAL(networksProgress(quint32, quint32)), this, SLOT(coreNetworksProgress(quint32, quint32)));
  //connect(syncer, SIGNAL(syncFinished()), this, SLOT(syncFinished()));
  //connect(syncer, SIGNAL(encrypted()), ui.secureConnection, SLOT(show()));
    printf("Connect\n");
    //ClientSyncer::connectToCore(map);
}

void Proxy::gotCoreSessionState(const QVariantMap& sessionState){
    syncronizing=true;
    foreach(QVariant idvar, sessionState["Identities"].toList()) {
          Identity idv=idvar.value<Identity>();
          if(!identities.contains(idv.id().toInt())) {
            Identity *identity = new Identity(idv, this);
            identities[idv.id().toInt()] = identity;
            identity->setInitialized();
            conn->getSignalProxy()->synchronize(identity);
            /*emit */identityRecieved(idv.id());
        }
    }
     // create buffers
  // FIXME: get rid of this crap
  QVariantList bufferinfos = sessionState["BufferInfos"].toList();
  //NetworkModel *networkModel = Client::networkModel();
  //Q_ASSERT(networkModel);

  foreach(QVariant vinfo, bufferinfos)
    bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

  QVariantList networkids = sessionState["NetworkIds"].toList();

  // prepare sync progress thingys...
  // FIXME: Care about removal of networks
  //numNetsToSync = networkids.count();
  //emit networksProgress(0, numNetsToSync);
  // create network objects
  foreach(QVariant networkid, networkids) {
    NetworkId netid = networkid.value<NetworkId>();
    if(networks.contains(netid.toInt()))
      continue;
    Network *net = new Network(netid, this);
    net->setProxy(conn->getSignalProxy());
    connect(net, SIGNAL(initDone()), this, SLOT(netsyncComplete()));
    netsToSync.insert(net);
    conn->getSignalProxy()->synchronize(net);
    networks.insert(netid.toInt(),net);
  }
  syncronizing=false;
}
void Proxy::netsyncComplete(){
    netsToSync.remove(sender());
    Network *net=qobject_cast<Network*>(sender());
    printf("Got network:%s\n",net->networkName().toUtf8().constData());
    //net->ircChannels()
    if(netsToSync.isEmpty()){
        printf("Syncronization complete\n");
        syncComplete();
    }
}
void Proxy::syncComplete(){
   /*foreach(BufferInfo binfo,bufferInfos.values()){
        if(binfo.type()==BufferInfo::ChannelBuffer){
            IrcChannel *chan=networks.value(binfo.networkId())->ircChannel(binfo.bufferName());
            if(!chan){
                printf("Channel not found for (%s): %s\n",networks.value(binfo.networkId())->networkName().toUtf8().constData(),binfo.bufferName().toUtf8().constData());
            }else{
                printf("Channel(%s): %s\n",networks.value(binfo.networkId())->networkName().toUtf8().constData(),chan->name().toUtf8().constData());
                foreach(IrcUser *usr, chan->ircUsers()){
                    printf("User:%s\n",usr->nick().toUtf8().constData());
                }
            }
        }
        //back->requestBacklog(bufferInfos.values()[5].buffer)
    }*/
  SignalProxy *p=conn->getSignalProxy();
  p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SLOT(recvMessage(const Message &)));
  //p->attachSlot(SIGNAL(displayStatusMsg(QString, QString)), this, SLOT(recvStatusMsg(QString, QString)));//unused

  p->attachSlot(SIGNAL(bufferInfoUpdated(BufferInfo)), this, SLOT(bufferUpdated(BufferInfo)));
  p->attachSignal(this, SIGNAL(sendInput(BufferInfo, QString)));
  p->attachSignal(this, SIGNAL(requestNetworkStates()));

  p->attachSignal(this, SIGNAL(requestCreateIdentity(const Identity &, const QVariantMap &)), SIGNAL(createIdentity(const Identity &, const QVariantMap &)));
  p->attachSignal(this, SIGNAL(requestRemoveIdentity(IdentityId)), SIGNAL(removeIdentity(IdentityId)));
  p->attachSlot(SIGNAL(identityCreated(const Identity &)), this, SLOT(coreIdentityCreated(const Identity &)));
  p->attachSlot(SIGNAL(identityRemoved(IdentityId)), this, SLOT(coreIdentityRemoved(IdentityId)));

  p->attachSignal(this, SIGNAL(requestCreateNetwork(const NetworkInfo &, const QStringList &)), SIGNAL(createNetwork(const NetworkInfo &, const QStringList &)));
  p->attachSignal(this, SIGNAL(requestRemoveNetwork(NetworkId)), SIGNAL(removeNetwork(NetworkId)));
  p->attachSlot(SIGNAL(networkCreated(NetworkId)), this, SLOT(coreNetworkCreated(NetworkId)));
  p->attachSlot(SIGNAL(networkRemoved(NetworkId)), this, SLOT(coreNetworkRemoved(NetworkId)));
  p->synchronize(back);
  //connect(back, SIGNAL(messagesReceived(BufferId, int)), this, SLOT(messagesReceived(BufferId, int)));
  connect(back, SIGNAL(backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList)), this, SLOT(backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList)));
  //BufferInfo reqbuf=bufferInfos.values()[5];
  //back->requestBacklog(reqbuf.bufferId(),MsgId(50),MsgId(2000),50,0);

  //respond to client, send setup, and all info.
  quasselproxy::Packet resp;
  resp.mutable_setup()->set_loggedin(true);
  resp.mutable_setup()->set_sessionid(setup.sessionid());
  resp.mutable_setup()->set_timebase(QDateTime().toTime_t());
  foreach(Identity *id,identities.values()){
      convertIdentity(id,resp.add_identities());
  }
  foreach(Network *network,networks.values()){
      convertNetworkToProto(network,resp.add_networks());
  }
  foreach(BufferInfo binfo,bufferInfos.values()){
      convertBufferInfoToProto(&binfo,resp.add_buffers());
  }
  foreach(ChannelInfo cinfo,channels.values()){
      if(networks.value(cinfo.nid)->ircChannel(cinfo.name)==NULL){
          printf("Channel not found in network:(%s)%s\n",networks.value(cinfo.nid)->networkName().toUtf8().constData(),cinfo.name.toUtf8().constData());
      }else{
          IrcChannel *chan=networks.value(cinfo.nid)->ircChannel(cinfo.name);
          connect(chan,SIGNAL(ircUsersJoined(QStringList,QStringList)),this,SLOT(ircUsersJoined(QStringList,QStringList)));
          connect(chan,SIGNAL(ircUsersJoined(QStringList, QStringList)),this,SLOT(ircUsersJoined(QStringList, QStringList)));
          connect(chan,SIGNAL(ircUserParted(IrcUser*)),this,SLOT(ircUserParted(IrcUser*)));
          connect(chan,SIGNAL(ircUserNickSet(IrcUser*,QString)),this,SLOT(ircUserNickSet(IrcUser*,QString)));
          connect(chan,SIGNAL(ircUserModeAdded(IrcUser*,QString)),this,SLOT(ircUserModeAdded(IrcUser*,QString)));
          connect(chan,SIGNAL(ircUserModeRemoved(IrcUser*,QString)),this,SLOT(ircUserModeRemoved(IrcUser*,QString)));
          connect(chan,SIGNAL(ircUserModesSet(IrcUser*,QString)),this,SLOT(ircUserModesSet(IrcUser*,QString)));
          convertIrcChannel(networks.value(cinfo.nid)->ircChannel(cinfo.name),resp.add_channels());
          quasselproxy::IrcChannel chanproto;
          convertIrcChannel(networks.value(cinfo.nid)->ircChannel(cinfo.name),&chanproto);
          if(!channelsInt.contains(cinfo)){
              printf("Adding not found channel (%s)%s\n",networks.value(cinfo.nid)->networkName().toUtf8().constData(),cinfo.name.toUtf8().constData());
          }
          quint32 cid= channelsInt.value(cinfo);
          channelClientInfo.insert(cid,chanproto);
          channelClientUsers.insert(cid,QList<quint32>());
      }
  }
  foreach(ChannelInfo uinfo,users.values()){
      if(networks.value(uinfo.nid)->ircUser(uinfo.name)==NULL){
          printf("User not found in network:(%s)%s\n",networks.value(uinfo.nid)->networkName().toUtf8().constData(),uinfo.name.toUtf8().constData());
      }else{
        convertIrcUserToProto(networks.value(uinfo.nid)->ircUser(uinfo.name),resp.add_users());
      }
  }

  sendPacket(resp);
}
void Proxy::convertBufferInfoToProto(const BufferInfo *binfo,quasselproxy::Buffer* proto){
    proto->set_bid(binfo->bufferId().toInt());
    proto->set_nid(binfo->networkId().toInt());
    proto->set_gid(binfo->groupId());
    proto->set_name(toStdStringUtf8(binfo->bufferName()));
    if(networks.contains(binfo->networkId().toInt())){
        qint32 nid=binfo->networkId().toInt();
        if((binfo->type()&BufferInfo::ChannelBuffer)==BufferInfo::ChannelBuffer){
            //IrcChannel *chan= networks.value(binfo->networkId())->ircChannel(binfo->bufferName());
            if(!channelsInt.contains(ChannelInfo(nid,binfo->bufferName()))){//add channel
                channels.insert(nextChanId,ChannelInfo(nid,binfo->bufferName()));//don't care about per network yet
                channelsInt.insert(ChannelInfo(nid,binfo->bufferName()),nextChanId);
                nextChanId++;
            }
            proto->set_channel(channelsInt.value(ChannelInfo(nid,binfo->bufferName())));
        }
        if((binfo->type()&BufferInfo::QueryBuffer)==BufferInfo::QueryBuffer){
            //IrcUser *usr= networks.value(binfo->networkId())->ircUser(binfo->bufferName());
            if(!usersInt.contains(ChannelInfo(nid,binfo->bufferName()))){//add user
                users.insert(nextUserId,ChannelInfo(nid,binfo->bufferName()));//don't care about per network yet
                usersInt.insert(ChannelInfo(nid,binfo->bufferName()),nextUserId);
                nextUserId++;
            }
            proto->set_queryuser(channelsInt.value(ChannelInfo(nid,binfo->bufferName())));
        }
    }
    //resolve user or channel depending on type
    //proto->set_firstmsg(binfo->
    proto->set_type_invalid((binfo->type()&BufferInfo::InvalidBuffer)==BufferInfo::InvalidBuffer);
    proto->set_type_status((binfo->type()&BufferInfo::StatusBuffer)==BufferInfo::StatusBuffer);
    proto->set_type_channel((binfo->type()&BufferInfo::ChannelBuffer)==BufferInfo::ChannelBuffer);
    proto->set_type_query((binfo->type()&BufferInfo::QueryBuffer)==BufferInfo::QueryBuffer);
    proto->set_type_group((binfo->type()&BufferInfo::GroupBuffer)==BufferInfo::GroupBuffer);
    if(bufferActivivty.contains(binfo->bufferId().toInt())){
        BufferInfo::ActivityLevel actLvl=BufferInfo::ActivityLevel(bufferActivivty.value(binfo->bufferId().toInt()));
        proto->set_activity_none(actLvl.testFlag(BufferInfo::NoActivity));
        proto->set_activity_other(actLvl.testFlag(BufferInfo::OtherActivity));
        proto->set_activity_newmessage(actLvl.testFlag(BufferInfo::NewMessage));
        proto->set_activity_highlight(actLvl.testFlag(BufferInfo::Highlight));
    }
}
void Proxy::convertNetworkToProto(const Network* net,quasselproxy::Network* proto){
    proto->set_nid(net->networkId().toInt());
    proto->set_name(toStdStringUtf8(net->networkName()));
    proto->set_idid(net->identity().toInt());
    proto->set_userandomserver(net->useRandomServer());
    foreach(QString perf,net->perform()){
        proto->add_perform(toStdStringUtf8(perf));
    }
    proto->set_useautoidentify(net->useAutoIdentify());
    proto->set_autoreconnectinterval(net->autoReconnectInterval());
    proto->set_unlimitedreconnectretries(net->unlimitedReconnectRetries());
    proto->set_rejoinchannels(net->rejoinChannels());
    switch(net->connectionState()){
        case Network::Disconnected:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Disconnected);
            break;
        case Network::Connecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Connecting);
            break;
        case Network::Initializing:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Initializing);
            break;
        case Network::Initialized:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Initialized);
            break;
        case Network::Reconnecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Reconnecting);
            break;
        case Network::Disconnecting:
            proto->set_connectionstate(quasselproxy::Network_ConnectionState_Disconnecting);
            break;
    }
    /*switch(net->channelModeType()){
        case Network::A_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_A_CHANMODE);
            break;
        case Network::B_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_B_CHANMODE);
            break;
        case Network::C_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_C_CHANMODE);
            break;
        case Network::D_CHANMODE:
            proto->set_chanmode(quasselproxy::Network_ChanMode_D_CHANMODE);
            break;
    }*/
}
void Proxy::convertIrcUserToProto(const IrcUser* usr,quasselproxy::IrcUser *proto){
  proto->set_nid(usr->network()->networkId().toInt());
  if(!usersInt.contains(ChannelInfo(usr->network()->networkId().toInt(),usr->nick()))){
      printf("User not found:(%s)%s\n",usr->network()->networkName().toUtf8().constData(),usr->nick().toUtf8().constData());
      return;
  }
  proto->set_uid(usersInt.value(ChannelInfo(usr->network()->networkId().toInt(),usr->nick())));
  proto->set_nick(toStdStringUtf8(usr->nick()));
  proto->set_user(toStdStringUtf8(usr->user()));
  proto->set_host(toStdStringUtf8(usr->host()));
  proto->set_realname(toStdStringUtf8(usr->realName()));
  //proto->set_awaymessage(toStdStringUtf8(usr->awayMessage()));//send this?
  proto->set_away(usr->isAway());
  proto->set_server(toStdStringUtf8(usr->server()));
  proto->set_logintime(usr->loginTime().toTime_t());
  proto->set_operator_(toStdStringUtf8(usr->ircOperator()));
  proto->set_suserhost(toStdStringUtf8(usr->suserHost()));
  proto->set_usermodes(toStdStringUtf8(usr->userModes()));
  foreach(QString chan,usr->channels())
      proto->add_channels(channelsInt.value(ChannelInfo(usr->network()->identity().toInt(),chan)));
  printf("Adduser:%s\n",usr->nick().toUtf8().constData());
}
void Proxy::convertIrcChannel(IrcChannel *chan,quasselproxy::IrcChannel *proto){
/*
  //Mode stuf, mirrors internal quassel stuf, not understood, used or implemented yet
  message AMode{
    required bytes mode=1;
    repeated string value=2;
  }
  message BCMode{
    required bytes mode=1;
    optional string value=2;
  }
  repeated AMode amodes=10;
  repeated BCMode bmodes=11;
  repeated BCMode cmodes=12;
  repeated bytes dmodes=13;

*/
    if(!channelsInt.contains(ChannelInfo(chan->network()->networkId().toInt(),chan->name()))){
        printf("Channel not found:(%s)%s\n",chan->network()->networkName().toUtf8().constData(),chan->name().toUtf8().constData());
        return;
    }
    proto->set_cid(channelsInt.value(ChannelInfo(chan->network()->networkId().toInt(),chan->name())));
    proto->set_nid(chan->network()->networkId().toInt());
    proto->set_name(toStdStringUtf8(chan->name()));
    proto->set_topic(toStdStringUtf8(chan->topic()));
    proto->set_password(toStdStringUtf8(chan->password()));
    //proto->set
    quint32 nid=chan->network()->networkId().toInt();
    foreach(IrcUser *usr,chan->ircUsers()){
        if(usersInt.contains(ChannelInfo(nid,usr->nick()))){//don't add user if it doesn't exist
            quasselproxy::IrcChannel::UserMode *um =proto->add_users();
            um->set_uid(usersInt.value(ChannelInfo(usr->network()->networkId().toInt(),usr->nick())));
            printf("SCID%d,%s;%s\n",um->uid(),usr->network()->networkName().toUtf8().constData(),usr->nick().toUtf8().constData());
            um->set_mode(toStdStringUtf8(chan->userModes(usr->nick())));
        }
    }
    //FIXME:Implement mode stuff
}
void Proxy::convertIdentity(const Identity *id,quasselproxy::Identity *proto){
    proto->set_idid(id->id().toInt());
    proto->set_identityname(toStdStringUtf8(id->identityName()));
    proto->set_realname(toStdStringUtf8(id->realName()));
    foreach(QString nick,id->nicks())
        proto->add_nicks(toStdStringUtf8(nick));
    proto->set_awaynick(toStdStringUtf8(id->awayNick()));
    proto->set_awaynickenabled(proto->awaynickenabled());
    proto->set_awayreason(toStdStringUtf8(id->awayReason()));
    proto->set_awayreasonenabled(id->awayReasonEnabled());
    proto->set_detatchawayenabled(id->detachAwayEnabled());
    proto->set_detatchawayreason(toStdStringUtf8(id->detachAwayReason()));
    proto->set_ident(toStdStringUtf8(id->ident()));
    proto->set_kickreason(toStdStringUtf8(id->kickReason()));
    proto->set_partreason(toStdStringUtf8(id->partReason()));
    proto->set_quitreason(toStdStringUtf8(id->quitReason()));
}
void Proxy::convertMessage(const Message *msg,quasselproxy::Message *proto){
    proto->set_msgid(msg->msgId().toInt());
    quint32 nid=bufferInfos.value(msg->bufferId().toInt()).networkId().toInt();
    //proto->set_nid(bufferInfos.value(msg->bufferId().toInt()).networkId().toInt());//client checks from buffer by itself
    if(msg->bufferId().toInt()!=activeBuffer)
        proto->set_bid(msg->bufferId().toInt());
    proto->set_uid(usersInt.value(ChannelInfo(nid,
                                             msg->sender().left(msg->sender().indexOf("!")>-1?
                                                                msg->sender().indexOf("!"):
                                                                msg->sender().length()))));//TODO: Check if work
    printf("Try lookup user:%s<\n", msg->sender().left(msg->sender().indexOf("!")>-1?
                                                                msg->sender().indexOf("!"):
                                                                msg->sender().length()).toUtf8().constData());
    proto->set_timestamp(msg->timestamp().toTime_t()-timebase);
    proto->set_contents(toStdStringUtf8(msg->contents()));
    proto->set_type(msg->type());
/*    proto->set_type_plain((msg->type()&Message::Plain)==Message::Plain);
    proto->set_type_notice((msg->type()&Message::Notice)==Message::Notice);
    proto->set_type_action((msg->type()&Message::Action)==Message::Action);
    proto->set_type_nick((msg->type()&Message::Nick)==Message::Nick);
    proto->set_type_mode((msg->type()&Message::Mode)==Message::Mode);
    proto->set_type_join((msg->type()&Message::Join)==Message::Join);
    proto->set_type_part((msg->type()&Message::Part)==Message::Part);
    proto->set_type_quit((msg->type()&Message::Quit)==Message::Quit);
    proto->set_type_kick((msg->type()&Message::Kick)==Message::Kick);
    proto->set_type_kill((msg->type()&Message::Kill)==Message::Kill);
    proto->set_type_server((msg->type()&Message::Server)==Message::Server);
    proto->set_type_info((msg->type()&Message::Info)==Message::Info);
    proto->set_type_error((msg->type()&Message::Error)==Message::Error);
    proto->set_type_daychange((msg->type()&Message::DayChange)==Message::DayChange);
*/
    proto->set_flag(msg->flags().operator int());
    /*proto->set_flag_self(msg->flags().testFlag(Message::Self));
    proto->set_flag_highlight(msg->flags().testFlag(Message::Highlight));
    proto->set_flag_redirected(msg->flags().testFlag(Message::Redirected));
    proto->set_flag_servermsg(msg->flags().testFlag(Message::ServerMsg));
    proto->set_flag_backlog(msg->flags().testFlag(Message::Backlog));*/
}
void Proxy::backlogRecieved(BufferId id , MsgId first, MsgId last, int limit, int additional, QVariantList messages){
    printf("Got backlog for:%s\n",bufferInfos.value(id.toInt()).bufferName().toUtf8().constData());
    foreach(QVariant msgv, messages){
        Message msg = msgv.value<Message>();
        printf("Blmsg:<%s>%s\n",msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());
    }
}
void Proxy::recvMessage(const Message& msg){
    if(!syncronizing&&!clientPause){
        if(msg.bufferId().toInt()==activeBuffer){
            quasselproxy::Packet pkg;
            convertMessage(&msg,pkg.add_messages());
            sendPacket(pkg);
        }
    }
    printf("MSG:<%s>%s\n",msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());

}
void Proxy::coreIdentityCreated(const Identity &){
}
void Proxy::coreIdentityRemoved(IdentityId){
}

void Proxy::coreNetworkCreated(NetworkId id){
}
void Proxy::coreNetworkRemoved(NetworkId id){
}
void Proxy::ircUsersJoined(QStringList nicks, QStringList modes){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    foreach(QString nick,nicks){
        printf("Joined:%s,%s\n",snd->name().toUtf8().constData(),nick.toUtf8().constData());
    }
}
void Proxy::ircUserParted(IrcUser *ircuser){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("Parted:%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData());
}
void Proxy::ircUserNickSet(IrcUser *ircuser, QString nick){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("Nickset:%s,%s->%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),nick.toUtf8().constData());
}
void Proxy::ircUserModeAdded(IrcUser *ircuser, QString mode){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeAdded:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),mode.toUtf8().constData());

}
void Proxy::ircUserModeRemoved(IrcUser *ircuser, QString mode){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeRemoved:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),mode.toUtf8().constData());
}
void Proxy::ircUserModesSet(IrcUser *ircuser, QString modes){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeSet:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),modes.toUtf8().constData());
}


void Proxy::bufferUpdated(BufferInfo info){
    if(bufferInfos.contains(info.bufferId().toInt())){
        printf("Updated buffer:%s,%d\n",info.bufferName().toUtf8().constData(),info.type());
        if(!syncronizing&&!clientPause){
            quasselproxy::Packet pkg;
            convertBufferInfoToProto(&info,pkg.add_buffers());
            sendPacket(pkg);
        }//FIXME: add to queue on pause
    }else{
        bufferInfos.insert(info.bufferId().toInt(),info);
        printf("Added buffer:%s,%d,%d\n",info.bufferName().toUtf8().constData(),info.type(),info.bufferId().toInt());
        if(!syncronizing&&clientPause){
            quasselproxy::Packet pkg;
            convertBufferInfoToProto(&info,pkg.add_buffers());
            sendPacket(pkg);
        }//FIXME: add to queue on pause
    }
}
void Proxy::identityRecieved(const IdentityId &id){
    printf("Id created%s\n",identities.value(id.toInt())->nicks().at(0).toUtf8().constData());
    if(!syncronizing&&clientPause){
            quasselproxy::Packet pkg;
            convertIdentity(identities.value(id.toInt()),pkg.add_identities());
            sendPacket(pkg);
    }//FIXME: add to queue on pause
}
void Proxy::disconnectedFromCore(){
}
void Proxy::startLogin(){
    //conn->startLogin();
    printf("Starting login\n");
    conn->loginToCore(fromStdStringUtf8(setup.username()),fromStdStringUtf8(setup.password()));
}
void Proxy::loginFailed(const QString &msg){
    printf("Login failed:%s\n",msg.toUtf8().constData());
      quasselproxy::Packet resp;//send fail message with setup?
      resp.mutable_setup()->set_loggedin(false);
      resp.mutable_setup()->set_loginfail(true);
      sendPacket(resp);
      client->flush();
      disconnect();
}
void Proxy::clientHasData(){
    int bufsize=0;
    if(newRead&&client->bytesAvailable()>=2){
        QByteArray in=client->read(2);
        if(((quint8)in.at(0))==255){//extended size
            in.append(client->read(2));//read the rest of the size, block if neccesary
            bufsize=(((quint8)in.at(1)) <<16)|(((quint8)in.at(2))<<8)|((quint8)in.at(3));
        }else{
            bufsize=(((quint8)in.at(0))<<8)|((quint8)in.at(1));
        }
        readBuf.resize(bufsize);
        bytesRead=0;
        newRead=false;
    }else if(newRead){
        return;
    }
    //printf("Toread%d:%d\n",bufsize,client->bytesAvailable());
    if(bufsize>10240&&!conn->isLoggedIn()){//FIXME: disconnect
        printf("Too big size, disconnecting\n");
        disconnect();
        return;
    }
    QByteArray in=client->read(min(client->bytesAvailable(),readBuf.size()-bytesRead));
    readBuf.replace(bytesRead,in.length(),in);
    bytesRead+=in.length();
    if(bytesRead==(quint32)readBuf.size()){
        //printf("Tryread\n");
        quasselproxy::Packet pkg=quasselproxy::Packet();
        parseFromByteArray(&pkg,readBuf);
        packetRecievedFromClient(pkg);
        bytesRead=0;
        newRead=true;
    }
}
void Proxy::packetRecievedFromClient(quasselproxy::Packet pkg){
    updateActivity();
    if(pkg.has_setup()){
        quint32 oldsid=setup.sessionid();
        //pkg.setup().sessionid()!=oldsid && pkg.setup().sessionid()!=0
        if(pkg.setup().has_sessionid()){//handle different sid's
            switchSid(pkg);//ask manager to switch session or disconnect
            return;//return as we are deleted now
        }else{//correct sid - start connecting etc.
            setup=pkg.setup();
            setup.set_sessionid(oldsid);
            init();
        }
    }
    if(pkg.has_statusinfo()){
        if(pkg.statusinfo().has_suspend() && pkg.statusinfo().suspend()==true){
            clientDisconnected=true;
            client->disconnectFromHost();
            client->close();
            client=NULL;
            return;
        }else if(pkg.statusinfo().has_paused() && pkg.statusinfo().paused()==true){
            clientPause=true;
        }else{
            if(pkg.statusinfo().activbid()!=activeBuffer){
                activateBuffer(pkg.statusinfo().activbid());
                activeBuffer=pkg.statusinfo().activbid();
            }
        }
    }
    for(int i=0;i<pkg.messages_size();i++){
        quasselproxy::Message msg=pkg.messages(i);
        if(!bufferInfos.contains(msg.bid()))
            printf("Could not send message for id, id not found:%d\n",msg.bid());
        else
            emit sendInput(bufferInfos.value(msg.bid()), fromStdStringUtf8(msg.contents()));
    }
}
void Proxy::activateBuffer(quint32 newbid){
    //process old buffer
    if(!bufferInfos.contains(newbid)){
        return;//don't change to a buffer that doesn't exist
    }
    quasselproxy::Packet pkg;
    //activeBuffer=newbid;
    BufferInfo binfo=bufferInfos.value(newbid);
    if((binfo.type()&BufferInfo::ChannelBuffer)==BufferInfo::ChannelBuffer){//send channel info
        if(networks.value(binfo.networkId().toInt())->ircChannel(binfo.bufferName())!=NULL){
            IrcChannel* chan=networks.value(binfo.networkId().toInt())->ircChannel(binfo.bufferName());
            foreach(IrcUser* usr,chan->ircUsers()){
                if(!usersInt.contains(ChannelInfo(binfo.networkId().toInt(),usr->nick()))){//add user if not existing
                    users.insert(nextUserId,ChannelInfo(binfo.networkId().toInt(),usr->nick()));//don't care about per network yet
                    usersInt.insert(ChannelInfo(binfo.networkId().toInt(),usr->nick()),nextUserId);
                    nextUserId++;
                    convertIrcUserToProto(usr,pkg.add_users());
                    printf("Adduser:%s\n",usr->nick().toUtf8().constData());
                }
            }
            if(channelsInt.contains(ChannelInfo(binfo.networkId().toInt(),chan->name()))){
                quint32 cid=channelsInt.value(ChannelInfo(binfo.networkId().toInt(),chan->name()));
                //quasselproxy::Buffer tb;
                quasselproxy::IrcChannel *retChan=pkg.add_channels();
                quasselproxy::IrcChannel genChan;
                quasselproxy::IrcChannel clientChan=channelClientInfo.value(cid);
                //make sure all users of channel is present first
                convertIrcChannel(chan,&genChan);
                //getDiff(&clientChan,retChan,&genChan);
                retChan->MergeFrom(genChan);
                //diff users
                printf("DFU c:%d,g:%d\n",clientChan.users_size(),genChan.users_size());
                bool inInner=false;
                for(int i=0;i<clientChan.users_size();i++){
                    inInner=false;
                    for(int j=0;j<genChan.users_size();j++){
                        if(clientChan.users(i).uid()==genChan.users(j).uid()){
                            inInner=true;
                            break;
                            printf("DFI:%d%,%d:%d,%d \n",i,j,clientChan.users(i).uid(),genChan.users(j).uid());
                        }else{
                            //printf("DFU:%d,%d:%d,%d \n",i,j,clientChan.users(i).uid(),genChan.users(j).uid());
                        }
                    }
                    if(inInner==false&&clientChan.users(i).uid()!=0){
                        retChan->add_usersparted()->MergeFrom(clientChan.users(i));
                        printf("Adduserparted:%d,%d\n",i,clientChan.users(i).uid());
                    }
                }
                for(int i=0;i<genChan.users_size();i++){
                    inInner=false;
                    for(int j=0;j<clientChan.users_size();j++){
                        if(genChan.users(i).uid()==clientChan.users(j).uid()){
                            inInner=true;
                            break;
                        }
                    }
                    if(inInner==false){
                        retChan->add_usersjoined()->MergeFrom(genChan.users(i));
                        printf("Adduserjoined:%d,%d\n",i,genChan.users(i).uid());
                    }
                }
                quasselproxy::IrcChannel insChan;
                insChan.MergeFrom(*retChan);
                channelClientInfo.insert(cid,insChan);
            }else{
                channels.insert(nextChanId,ChannelInfo(binfo.networkId().toInt(),chan->name()));//don't care about per network yet
                channelsInt.insert(ChannelInfo(binfo.networkId().toInt(),chan->name()),nextChanId);
                quint32 cid=nextChanId;
                nextChanId++;
                quasselproxy::IrcChannel *retChan=pkg.add_channels();
                convertIrcChannel(chan,retChan);
            }
        }
    }else if((binfo.type()&BufferInfo::QueryBuffer)==BufferInfo::QueryBuffer){
        if(!usersInt.contains(ChannelInfo(binfo.networkId().toInt(),binfo.bufferName()))){//add user if not existing
            users.insert(nextUserId,ChannelInfo(binfo.networkId().toInt(),binfo.bufferName()));//don't care about per network yet
            usersInt.insert(ChannelInfo(binfo.networkId().toInt(),binfo.bufferName()),nextUserId);
            nextUserId++;
        }
    }
    printf("Send buffer change packet%d\n",binfo.type());
    pkg.mutable_statusinfo()->set_activbid(newbid);
    sendPacket(pkg);
}
void Proxy::sendPacket(quasselproxy::Packet pkg){
    //stop packet where when paused?
    if(clientPause){
        printf("Warning, sending packet when client is paused\n");
    }
    if(client==NULL || !client->isOpen()){
        if(clientDisconnected){
            printf("Warning, sending packet when client is suspended\n");
            return;
        }else{
            disconnect();
        }
    }
    int len=pkg.ByteSize();
    if(len>32768){//works with sizes up to 16Mb
            quint8 sz[4];
            sz[0]=(quint8)255;
            sz[1]=(quint8)((len>>16)&255);
            sz[2]=(quint8)((len>>8)&255);
            sz[3]=(quint8)((len)&255);
            client->write((char*)sz,4);
            printf("SNDPKGE:%d,%d,%d,%d,%d\n",len,sz[0],sz[1],sz[2],sz[3]);
    }else{
            quint8 sz[2];
            sz[0]=(quint8)((len>>8)&255);
            sz[1]=(quint8)((len)&255);
            client->write((char*)sz,2);
            printf("SNDPKG:%d,%d,%d\n",len,sz[0],sz[1]);
    }
    client->write(serializeToByteArray(&pkg));
}
void getDiff(const google::protobuf::Message *m1,const google::protobuf::Message *m2,google::protobuf::Message *mt){
    const google::protobuf::Descriptor *d1=m1->GetDescriptor();
    const google::protobuf::Descriptor *d2=m2->GetDescriptor();
    const google::protobuf::Descriptor *dt=mt->GetDescriptor();
    for(int i=0;i<d1->field_count();i++){
       const google::protobuf::FieldDescriptor *f1=d1->field(i);
       const google::protobuf::FieldDescriptor *f2=d2->field(i);
       const google::protobuf::FieldDescriptor *mf=dt->field(i);
       if(f1->type()==google::protobuf::FieldDescriptor::TYPE_STRING){
            if(m1->GetReflection()->GetString(*m1,f1)!=
               m2->GetReflection()->GetString(*m2,f2)){
                mt->GetReflection()->SetString(mt,mf,m2->GetReflection()->GetString(*m2,d1->field(i)));
            }
        }
       if(f1->type()==google::protobuf::FieldDescriptor::TYPE_UINT32){
            if(m1->GetReflection()->GetUInt32(*m1,f1)!=
               m2->GetReflection()->GetUInt32(*m2,f2)){
                mt->GetReflection()->SetUInt32(mt,mf,m2->GetReflection()->GetUInt32(*m2,d1->field(i)));
            }
        }
    }
}
void Proxy::setSid(int sid){
    setup.set_sessionid(sid);
}
int Proxy::getSid(){
    return setup.sessionid();
}
void Proxy::disconnect(){//Disconnect and destroy this session
    activityChecker.stop();
    if(client!=NULL && client->isOpen()){
        ((QObject)this).disconnect(client,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
        ((QObject)this).disconnect(client,SIGNAL(readyRead()),this,SLOT(clientHasData()));
        client->disconnectFromHost();
        client->close();//FIXME:crash
        conn->disconnectFromCore();

        delete(client);
        client=NULL;
    }
    emit removeSession();
}
void Proxy::updateActivity(){
    lastActivity=QDateTime::currentDateTime();
}
void Proxy::checkActivity(){
    if(!clientDisconnected && client!=NULL && client->isOpen()){
        if(lastActivity.secsTo(QDateTime::currentDateTime())>MAX_IDLE_TIME){
            //disconnect();
            printf("Warning: Would disconnect here\n");
        }
    }
}
bool Proxy::validateCreditals(quasselproxy::SessionSetup setup){
    //printf("CKPSW:%s=%s&&%s=%s\n",setup.username().c_str(),this->setup.username().c_str(),
    //       setup.password().c_str(),this->setup.password().c_str());
    return setup.username()==this->setup.username() &&
           setup.password()==this->setup.password();
}
bool Proxy::hasClient(){
    return client!=NULL && client->isOpen();
}
QTcpSocket *Proxy::getClientSocket(){
    return client;
}
QTcpSocket *Proxy::takeClientSocket(){
    QTcpSocket *cb=client;
    ((QObject)this).disconnect(client,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    ((QObject)this).disconnect(client,SIGNAL(readyRead()),this,SLOT(clientHasData()));
    client=NULL;
    return cb;
}
void Proxy::giveClientSocket(QTcpSocket *sock){
    connect(sock,SIGNAL(readyRead()),this,SLOT(clientHasData()));
    connect(sock,SIGNAL(disconnected()),this,SLOT(socketDisconnected()));
    client=sock;
    clientDisconnected=false;
}
void Proxy::socketDisconnected(){
    if(!clientDisconnected){
        disconnect();//disconnect from core as the socket was disconnected w/o asking for suspending session
    }
}
