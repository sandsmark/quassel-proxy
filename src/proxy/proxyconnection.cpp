//Copyright GPLv2/3  Frederik M.J.V. 
#include "proxyconnection.h"
#include "proxymessageprocessor.h"
#include "chatlinemodel.h"
#include "connectionmanager.h"
#include "identity.h"
#include "bufferinfo.h"
#include "backlogrequester.h"
#include "protocol.pb.h"
#include "prototools.h"
#include "protoconvert.h"
#include "proxyapplication.h"
#include "proxyuser.h"
#include <cstdlib>
#include <QDateTime>
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#define MAX_IDLE_TIME 300
#define MAX_PRECON_IDLE_TIME 10
void getDiff(const google::protobuf::Message *m1,const google::protobuf::Message *m2,google::protobuf::Message *mt);
ProxyConnection::ProxyConnection(QTcpSocket *client,ProxyApplication *app){
    this->app=app;
    this->client=client;
    this->conn=NULL;
    clientPersistentInfoVersion=-1;
    connect(client,SIGNAL(disconnected()),this,SLOT(disconnect()));
    connect(client,SIGNAL(readyRead()),this,SLOT(clientHasData()));
    activeBuffer=0;
    newRead=true;
    clientPause=false;
    clientDisconnected=false;
    syncronizing=false;
    authenticatedWithCore=false;
    lastActivity=QDateTime::currentDateTime();
    activityChecker.setInterval(5000);
    connect(&activityChecker,SIGNAL(timeout()),this,SLOT(checkActivity()));
    activityChecker.start();
    timebase=0;
    //setup.set_sessionid(sid);
}
ProxyConnection::~ProxyConnection(){
    activityChecker.stop();
    if(client!=NULL && client->isOpen()){
        ((QObject)this).disconnect(client,SIGNAL(disconnected()),this,SLOT(disconnect()));
        ((QObject)this).disconnect(client,SIGNAL(readyRead()),this,SLOT(clientHasData()));
        client->disconnectFromHost();
        client->close();//FIXME:crash
        delete(client);
        client=NULL;
    }
    if(conn)
        conn->deregisterConnection(this);
    //delete conn;
}
void ProxyConnection::connectUserSignals(){
    connect(conn,SIGNAL(recvMessage(const Message &)),this, SLOT(recvMessage(const Message &)));
    connect(conn,SIGNAL(backlogRecieved(BufferId,MsgId,MsgId,int,int,QVariantList)),this, SLOT(backlogRecieved(BufferId,MsgId,MsgId,int,int,QVariantList)));
    connect(conn,SIGNAL(bufferUpdated(BufferInfo)),this, SLOT(bufferUpdated(BufferInfo)));
    connect(conn,SIGNAL(updatePersistentInfo()),this,SLOT(updatePersistentInfo()));
    connect(this,SIGNAL(sendInput(BufferInfo,QString)),conn,SIGNAL(sendInput(BufferInfo,QString)));
}
void ProxyConnection::syncComplete(){
  connectUserSignals();
  authenticatedWithCore=true;
  quasselproxy::Packet resp;
  resp.mutable_setup()->set_loggedin(true);
  timebase=QDateTime::currentDateTime().toTime_t();
  resp.mutable_setup()->set_timebase(timebase);
  printf("ID's:%d,%d\n",clientPersistentInfoVersion,conn->getSid());
  if(clientPersistentInfoVersion!=conn->getSid()){//send new persistent info etc.
      generatePersistentInfo(&resp);
      printf("Send info\n");
  }else{
      resp.mutable_setup()->set_sessionid(clientPersistentInfoVersion);
  }
  if(activeBuffer){//send new messages from active buffer
      generateActivateBufferPacket(activeBuffer,&resp,lastSendtMessage);
      printf("Send active buffer\n");
  }
  printf("Sync complete %d",clientPersistentInfoVersion);
  sendPacket(resp);
  //send buffer messages?
}

void ProxyConnection::updatePersistentInfo(){
    quasselproxy::Packet resp;
    generatePersistentInfo(&resp);
    sendPacket(resp);

}
void ProxyConnection::generatePersistentInfo(quasselproxy::Packet *resp){
  //respond to client, send setup, and all info.
  clientPersistentInfoVersion=conn->getSid();
  foreach(Identity *id,conn->getIdentities()->values()){
      convertIdentity(id,resp->add_identities());
  }
  foreach(Network *network,conn->getNetworks()->values()){
      convertNetworkToProto(network,resp->add_networks());
  }
  foreach(BufferInfo binfo,conn->getBufferInfos()->values()){
      quasselproxy::Buffer *bproto=resp->add_buffers();
      convertBufferInfoToProto(&binfo,bproto);
      if(conn->getNetworks()->contains(binfo.networkId().toInt()) &&
         conn->getNetworks()->value(binfo.networkId().toInt())->ircChannel(binfo.bufferName())){
          bproto->set_topic(toStdStringUtf8(conn->getNetworks()->value(binfo.networkId().toInt())->ircChannel(binfo.bufferName())->topic()));
      }
  }
  resp->mutable_setup()->set_sessionid(clientPersistentInfoVersion);
}
void ProxyConnection::activateBuffer(quint32 newbid,quint32 lastSeenMsg){
    quasselproxy::Packet resp;
    if(conn->getBufferInfos()->contains(newbid)){
        generateActivateBufferPacket(newbid,&resp,lastSeenMsg);
    }else{
        quasselproxy::Packet spkg;
        spkg.mutable_statusinfo()->set_errormsg("Buffer does not exist");
        generatePersistentInfo(&resp);
    }
    sendPacket(resp);
}
void ProxyConnection::generateActivateBufferPacket(quint32 newbid,quasselproxy::Packet *resp,quint32 lastSeenMsg){
    //process old buffer
    if(!conn->getBufferInfos()->contains(newbid)){
        return;//don't change to a buffer that doesn't exist
    }
    //activeBuffer=newbid;
    BufferInfo binfo=conn->getBufferInfos()->value(newbid);
    convertBufferInfoToProto(&binfo,resp->add_buffers());
    /*if(lastSendtMessage!=-1){//send BUF_MSG's messages in history, or until last seen msg - problems with asyncronous backlog fetcher
    }*/
    printf("Send buffer change packet%d:%d\n",binfo.type(),newbid);
    resp->mutable_statusinfo()->set_activbid(newbid);
}

void ProxyConnection::backlogRecieved(BufferId id, MsgId first, MsgId last, int limit, int additional, QVariantList messages){
    printf("Got backlog for(%d):%s;%d\n",id.toInt(),conn->getBufferInfos()->value(id.toInt()).bufferName().toUtf8().constData(),messages.length());
    if(messages.length()<1)//don't bother with this then
        return;
    quasselproxy::Packet pkg;
    foreach(QVariant msgv, messages){
        Message msg = msgv.value<Message>();
        convertMessage(&msg,pkg.add_messages());
        printf("Blmsg:<%s>%s\n",msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());
    }
    sendPacket(pkg);
}
void ProxyConnection::recvMessage(const Message& msg){
    //bool seen=false;
    if(!syncronizing&&!clientPause){
        if(msg.bufferId().toInt()==activeBuffer){
            quasselproxy::Packet pkg;
            convertMessage(&msg,pkg.add_messages());
            sendPacket(pkg);
            
            emit lastSeenMsg(msg.bufferId(),msg.msgId());
            //seen=true;
            //printf("SEEN);
        }else{//should we send activity info
            if(conn->isForUs(msg)){
                if(!(bufferStates.contains(msg.bufferId().toInt())&&
                    bufferStates.value(msg.bufferId().toInt())==BufferInfo::Highlight)){
                    bufferStates.insert(msg.bufferId().toInt(),BufferInfo::Highlight);
                    //send higlight message
                    quasselproxy::Packet pkg;
                    quasselproxy::Buffer* bufmsg= pkg.add_buffers();
                    bufmsg->set_bid(msg.bufferId().toInt());
                    bufmsg->set_activity_highlight(true);
                    sendPacket(pkg);
                    printf("HighMsg:(%d)\n",msg.bufferId().toInt());
                }
            }else if(bufferStates.contains(msg.bufferId().toInt())&&
                     (bufferStates.value(msg.bufferId().toInt())==BufferInfo::NoActivity||
                     bufferStates.value(msg.bufferId().toInt())==BufferInfo::OtherActivity)){
                    bufferStates.insert(msg.bufferId().toInt(),BufferInfo::NewMessage);
                    //send newmessage message
                    quasselproxy::Packet pkg;
                    quasselproxy::Buffer* bufmsg= pkg.add_buffers();
                    bufmsg->set_bid(msg.bufferId().toInt());
                    bufmsg->set_activity_newmessage(true);
                    sendPacket(pkg);
                    printf("NewMsg:(%d)\n",msg.bufferId().toInt());
            }
        }
    }
    printf("MSG:(%d,%d)%s>%s\n",msg.bufferId().toInt(),msg.msgId().toInt(),msg.sender().toUtf8().constData(),msg.contents().toUtf8().constData());

}
//void setLastSeenMsg(BufferId, MsgId);

void ProxyConnection::bufferUpdated(BufferInfo info){
    if(!syncronizing&&!clientPause){
        quasselproxy::Packet pkg;
        convertBufferInfoToProto(&info,pkg.add_buffers());
        sendPacket(pkg);
    }//FIXME: add to queue on pause
}
void ProxyConnection::disconnectedFromCore(){
    disconnect();
}
void ProxyConnection::loginFailed(){
    //printf("Login failed:%s\n",msg.toUtf8().constData());
      quasselproxy::Packet resp;//send fail message with setup?
      resp.mutable_setup()->set_loggedin(false);
      resp.mutable_setup()->set_loginfail(true);
      sendPacket(resp);
      client->flush();
      disconnect();
}
void ProxyConnection::setSid(int sid){
    clientPersistentInfoVersion=sid;
}
int ProxyConnection::getSid(){
    return clientPersistentInfoVersion;
}
QString ProxyConnection::getUsername(){
    return fromStdStringUtf8(setup.username());
}

void ProxyConnection::authenticateClient(quasselproxy::Packet resp){
    quint32 oldsid=setup.sessionid();
    ProxyUser *proxyUser=app->getSession(fromStdStringUtf8(resp.setup().username()));
    bool newsession=false;
    clientPersistentInfoVersion=resp.setup().sessionid();
    printf("Clientid:%d\n",clientPersistentInfoVersion);
    if(proxyUser!=NULL){//existing session found
        if(!proxyUser->validateCreditals(resp.setup())){//not correct creditals
            quasselproxy::Packet spkg;
            resp.mutable_setup()->set_loggedin(false);
            resp.mutable_setup()->set_loginfail(true);
            sendPacket(spkg);
            client->flush();
            disconnect();
            return;
        }
        conn=proxyUser;
        conn->registerConnection(this);
        printf("Exsiting session %s\n",resp.setup().username().c_str());
        syncComplete();
    }else{
        conn=new ProxyUser(app);
        conn->registerConnection(this);//resp.setup().username(),resp.setup().password(),this);
        conn->setCreditals(resp.setup());
        app->registerSession(conn);//FIXME: Figure out a genious way to avoid dos by bad passwords
        conn->init();
        printf("New session %s\n",resp.setup().username().c_str());
    }
}
void ProxyConnection::packetRecievedFromClient(quasselproxy::Packet pkg){
    updateActivity();

    //check authentication
    if(conn==NULL){//i.e. has not authenticated
        if(pkg.has_setup()){
            authenticateClient(pkg);
        }else{
            //Do some pretty suicide
            quasselproxy::Packet spkg;
            spkg.mutable_statusinfo()->set_errormsg("Protocol error, the client has to authenticate first");
            sendPacket(spkg);
            disconnect();
            return;
        }
    }

    //check statusinfo
    if(pkg.has_statusinfo()){
        if(pkg.statusinfo().has_stopsession() && pkg.statusinfo().stopsession()==true){
            //clientDisconnected=true;
            disconnect();
            return;
        }else if(pkg.statusinfo().has_paused() && pkg.statusinfo().paused()==true){
            clientPause=true;
        }else{
            if(pkg.statusinfo().activbid()!=activeBuffer){
                bufferStates.insert(activeBuffer,BufferInfo::NoActivity);
                activateBuffer(pkg.statusinfo().activbid());
                activeBuffer=pkg.statusinfo().activbid();
            }
        }
    }

    //check backlogrequests
    int i;
    for(i=0;i<pkg.buffers_size();i++){//requests for backlog
        quasselproxy::Buffer msg=pkg.buffers(i);
        int first=msg.firstmsg();
        int last=msg.lastmsg();
        int limit=msg.nummsgs();
        if(first==0)
            first=-1;
        if(last==0)
            last=-1;
        if(limit==0)
            limit=-1;
        //back->requestBacklog(bufferInfos.values()[5].buffer);
        //printf("Reqbacklog:%d,%d,%d,%d,0\n",msg.bid(),first,last,limit);
        conn->getBacklogRequester()->requestBacklog(msg.bid(),first,last,limit,0);
    }

    //check for messages to send
    for(int i=0;i<pkg.messages_size();i++){
        quasselproxy::Message msg=pkg.messages(i);
        printf("Gotmsg:%s\n",msg.contents().c_str());
        if(!conn->getBufferInfos()->contains(msg.bid()))
            printf("Could not send message for id, id not found:%d\n",msg.bid());
        else
            emit sendInput(conn->getBufferInfos()->value(msg.bid()), fromStdStringUtf8(msg.contents()));
    }
}


//Socket communication stuff

void ProxyConnection::clientHasData(){
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
void ProxyConnection::sendPacket(quasselproxy::Packet pkg){
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
    client->waitForBytesWritten(50);
}

//Activity stuff
void ProxyConnection::disconnect(){//Disconnect and destroy this session
    deleteLater();
}
void ProxyConnection::updateActivity(){
    lastActivity=QDateTime::currentDateTime();
}
void ProxyConnection::checkActivity(){
    if(client!=NULL && client->isOpen()){
        if(!authenticatedWithCore && lastActivity.secsTo(QDateTime::currentDateTime())>MAX_PRECON_IDLE_TIME){
            disconnect();
        }else if(lastActivity.secsTo(QDateTime::currentDateTime())>MAX_IDLE_TIME){
            printf("Warning: Disconnecting from client due to ping timeout\n");
            disconnect();
        }
    }
}
bool ProxyConnection::validateCreditals(quasselproxy::SessionSetup setup){
    //printf("CKPSW:%s=%s&&%s=%s\n",setup.username().c_str(),this->setup.username().c_str(),
    //       setup.password().c_str(),this->setup.password().c_str());
    return setup.username()==this->setup.username() &&
           setup.password()==this->setup.password();
}
bool ProxyConnection::hasClient(){
    return client!=NULL && client->isOpen();
}
/*Must be here to take advantage of active buffer etc*/
void ProxyConnection::convertMessage(const Message *msg,quasselproxy::Message *proto){
    proto->set_msgid(msg->msgId().toInt());
    quint32 nid=conn->getBufferInfos()->value(msg->bufferId().toInt()).networkId().toInt();
    //proto->set_nid(bufferInfos.value(msg->bufferId().toInt()).networkId().toInt());//client checks from buffer by itself
    if(msg->bufferId().toInt()!=activeBuffer)
        proto->set_bid(msg->bufferId().toInt());
    proto->set_nick(toStdStringUtf8(msg->sender().left(msg->sender().indexOf("!")>-1?
                                                                msg->sender().indexOf("!"):
                                                                msg->sender().length())));
    /*printf("Try lookup user:%s<\n", msg->sender().left(msg->sender().indexOf("!")>-1?
                                                                msg->sender().indexOf("!"):
                                                                msg->sender().length()).toUtf8().constData());*/
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

