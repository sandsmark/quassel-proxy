//Copyright GPLv2/3  Frederik M.J.V. 
#include "proxyuser.h"
#include "proxymessageprocessor.h"
#include "chatlinemodel.h"
#include "connectionmanager.h"
#include "identity.h"
#include "bufferinfo.h"
#include "backlogrequester.h"
#include "protocol.pb.h"
#include "prototools.h"
#include "proxyapplication.h"
#include "proxyconnection.h"
#include <cstdlib>
#include <QDateTime>
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif
#define MAX_IDLE_TIME 300
#define MAX_PRECON_IDLE_TIME 10
#define PERSISTENT_ID_MAX 10000
void getDiff(const google::protobuf::Message *m1,const google::protobuf::Message *m2,google::protobuf::Message *mt);
ProxyUser::ProxyUser(ProxyApplication *app){
    this->app=app;
    conn=new ConnectionManager();
    back=new BacklogRequester();
    syncronizing=false;
    authenticatedWithCore=false;
    clientPersistentInfoVersion=rand()/(RAND_MAX/PERSISTENT_ID_MAX);//Generate "unique" persistent info id
}
ProxyUser::~ProxyUser(){
    app->removeSession(this);
    QList<QPointer<ProxyConnection> > connTemp(clientConenections);
    for(int i=0;i<connTemp.length();i++){
        delete(connTemp.at(i));
    }
    clientConenections.clear();
    if(conn)
        conn->disconnectFromCore();
}

MessageModel *ProxyUser::createMessageModel(QObject *parent){
  return new ChatLineModel(parent);
}
AbstractMessageProcessor *ProxyUser::createMessageProcessor(QObject *parent){
  return new ProxyMessageProcessor(parent,this);
}


void ProxyUser::init(){
    QVariantMap map;
    map.insert("Host",app->getCoreHost());
    map.insert("Port",app->getCorePort());
    conn->connectToCore(map);
    //FIXME: Implement SSL
    //syncer = new ClientSyncer(this);
    //connect(syncer, SIGNAL(connectionError(const QString &)), this, SLOT(initPhaseError(const QString &)));
    connect(conn, SIGNAL(startLogin()), this, SLOT(startLogin()));
    connect(conn, SIGNAL(connectionEstablished()), this, SLOT(connectedToCore()));
    connect(conn, SIGNAL(gotCoreSessionState(const QVariantMap&)), this, SLOT(gotCoreSessionState(const QVariantMap&)));
    connect(conn, SIGNAL(loginFailed(const QString &)), this, SLOT(loginFailed(const QString &)));
    printf("Connecting\n");
}

void ProxyUser::gotCoreSessionState(const QVariantMap& sessionState){
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
void ProxyUser::netsyncComplete(){
    netsToSync.remove(sender());
    Network *net=qobject_cast<Network*>(sender());
    printf("Got network:%s\n",net->networkName().toUtf8().constData());
    //net->ircChannels()
    if(netsToSync.isEmpty()){
        printf("Syncronization complete\n");
        syncComplete();
    }
}
void ProxyUser::syncComplete(){
  authenticatedWithCore=true;
  SignalProxy *p=conn->getSignalProxy();
  p->attachSlot(SIGNAL(displayMsg(const Message &)), this, SIGNAL(recvMessage(const Message &)));
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
  connect(back, SIGNAL(backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList)), this, SIGNAL(backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList)));
  //BufferInfo reqbuf=bufferInfos.values()[5];
  //back->requestBacklog(reqbuf.bufferId(),MsgId(50),MsgId(2000),50,0);
  foreach(ProxyConnection* clientConn, clientConenections){
    clientConn->syncComplete();
  }
}

void ProxyUser::updatePersistentInfoPrivate(){
    clientPersistentInfoVersion=rand()/(RAND_MAX/PERSISTENT_ID_MAX);//ID_MAX=1000
    emit updatePersistentInfo();
}

void ProxyUser::coreIdentityCreated(const Identity &){
    updatePersistentInfoPrivate();
}
void ProxyUser::coreIdentityRemoved(IdentityId){
    updatePersistentInfoPrivate();
}

void ProxyUser::coreNetworkCreated(NetworkId id){
    updatePersistentInfoPrivate();
}
void ProxyUser::coreNetworkRemoved(NetworkId id){
    updatePersistentInfoPrivate();
}
void ProxyUser::ircUsersJoined(QStringList nicks, QStringList modes){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    foreach(QString nick,nicks){
        printf("Joined:%s,%s\n",snd->name().toUtf8().constData(),nick.toUtf8().constData());
    }
}
void ProxyUser::ircUserParted(IrcUser *ircuser){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("Parted:%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData());
}
void ProxyUser::ircUserNickSet(IrcUser *ircuser, QString nick){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("Nickset:%s,%s->%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),nick.toUtf8().constData());
}
void ProxyUser::ircUserModeAdded(IrcUser *ircuser, QString mode){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeAdded:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),mode.toUtf8().constData());

}
void ProxyUser::ircUserModeRemoved(IrcUser *ircuser, QString mode){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeRemoved:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),mode.toUtf8().constData());
}
void ProxyUser::ircUserModesSet(IrcUser *ircuser, QString modes){
    IrcChannel *snd =qobject_cast<IrcChannel*>(sender());
    printf("ModeSet:%s,%s,%s\n",snd->name().toUtf8().constData(),ircuser->nick().toUtf8().constData(),modes.toUtf8().constData());
}


void ProxyUser::identityRecieved(const IdentityId &id){
    printf("Id created%s\n",identities.value(id.toInt())->nicks().at(0).toUtf8().constData());
    updatePersistentInfoPrivate();
}
void ProxyUser::disconnectedFromCore(){
    //FIXME: Test that this is really called
    disconnect();
}
void ProxyUser::startLogin(){
    //conn->startLogin();
    printf("Starting login\n");
    conn->loginToCore(username,password);
}
void ProxyUser::loginFailed(const QString &msg){
    //foreach(ProxyConnection* clientConn, clientConenections){
    for(int i=0;i<clientConenections.length();i++){
        clientConenections.at(i)->loginFailed();
        clientConenections[i]=NULL;
    }
    clientConenections.clear();
      disconnect();
}
QString ProxyUser::getUsername(){
    return username;
}
void ProxyUser::disconnect(){//Disconnect and destroy this session
    deleteLater();
}
bool ProxyUser::validateCreditals(quasselproxy::SessionSetup setup){
    //printf("CKPSW:%s=%s&&%s=%s\n",setup.username().c_str(),this->setup.username().c_str(),
    //       setup.password().c_str(),this->setup.password().c_str());
    return fromStdStringUtf8(setup.username())==username &&
           fromStdStringUtf8(setup.password())==password;
}
bool ProxyUser::setCreditals(quasselproxy::SessionSetup setup){
    if(!(username.isEmpty()||password.isEmpty()))
        return false;
    username=fromStdStringUtf8(setup.username());
    password=fromStdStringUtf8(setup.password());
    return true;
}

bool ProxyUser::hasClient(){
    return clientConenections.length()!=0;
}
int ProxyUser::getSid(){
    return clientPersistentInfoVersion;
}
void ProxyUser::setSid(int sid){
    clientPersistentInfoVersion=sid;
}
void ProxyUser::registerConnection(ProxyConnection* conn){
    if(!clientConenections.contains(conn))
        clientConenections.append(conn);
}
void ProxyUser::deregisterConnection(ProxyConnection* conn){
    clientConenections.removeAll(conn);
}
QHash<quint32, Identity *>* ProxyUser::getIdentities(){
  return &identities;
}
QHash<quint32, BufferInfo>* ProxyUser::getBufferInfos(){
  return &bufferInfos;
}
QHash<quint32, Network* >* ProxyUser::getNetworks(){
  return &networks;
}
bool ProxyUser::isLoggedIn(){
    return authenticatedWithCore;
}
QPointer<BacklogRequester> ProxyUser::getBacklogRequester(){
    return back;
}
