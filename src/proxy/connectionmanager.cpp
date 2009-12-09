#include "connectionmanager.h"
#include <QTcpSocket>
#include <QSslSocket>
#include <QNetworkProxy>
#include "signalproxy.h"
#include "quassel.h"

ConnectionManager::ConnectionManager()
{
    signalProxy= new SignalProxy(this);
    _blockSize=0;
}
ConnectionManager::~ConnectionManager(){
    delete(signalProxy);
}
void ConnectionManager::resetConnection(){
  if(socket) {
    disconnect(socket, 0, this, 0);
    socket->deleteLater();
    socket = 0;
  }
  _blockSize = 0;

  coreConnectionInfo.clear();
  _coreMsgBuffer.clear();

  //netsToSync.clear();
  //numNetsToSync = 0;

}

void ConnectionManager::connectToCore(QVariantMap &conn){
  resetConnection();
  coreConnectionInfo = conn;

  if(conn["Host"].toString().isEmpty()) {
    emit connectionError(tr("No Host to connect to specified."));
    return;
  }

  Q_ASSERT(!socket);
#ifdef HAVE_SSL
  QSslSocket *sock = new QSslSocket(this);
#else
  if(conn["useSsl"].toBool()) {
    emit connectionError(tr("<b>This client is built without SSL Support!</b><br />Disable the usage of SSL in the account settings."));
    return;
  }
  QTcpSocket *sock = new QTcpSocket(this);
#endif

#ifndef QT_NO_NETWORKPROXY
  if(conn.contains("useProxy") && conn["useProxy"].toBool()) {
    QNetworkProxy proxy((QNetworkProxy::ProxyType)conn["proxyType"].toInt(), conn["proxyHost"].toString(), conn["proxyPort"].toUInt(), conn["proxyUser"].toString(), conn["proxyPassword"].toString());
    sock->setProxy(proxy);
  }
#endif

  socket = sock;
  connect(sock, SIGNAL(readyRead()), this, SLOT(coreHasData()));
  connect(sock, SIGNAL(connected()), this, SLOT(connectedToCore()));
  connect(sock, SIGNAL(disconnected()), this, SLOT(disconnectedFromCore()));
  connect(sock, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
  connect(sock, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
  sock->connectToHost(conn["Host"].toString(), conn["Port"].toUInt());

}
void ConnectionManager::loginToCore(QString user,QString password){
  emit connectionMsg(tr("Logging in..."));
  QVariantMap clientLogin;
  clientLogin["MsgType"] = "ClientLogin";
  clientLogin["User"] = user;
  clientLogin["Password"] = password;
  SignalProxy::writeDataToDevice(socket, clientLogin);
}
void ConnectionManager::connectedToCore(){
    //emit startLogin();
  QVariantMap clientInit;
  clientInit["MsgType"] = "ClientInit";
  clientInit["ClientVersion"] = Quassel::buildInfo().fancyVersionString;
  clientInit["ClientDate"] = Quassel::buildInfo().buildDate;
  clientInit["ProtocolVersion"] = Quassel::buildInfo().protocolVersion;
  clientInit["UseSsl"] = coreConnectionInfo["useSsl"];
#ifndef QT_NO_COMPRESS
  clientInit["UseCompression"] = true;
#else
  clientInit["UseCompression"] = false;
#endif
  SignalProxy::writeDataToDevice(socket, clientInit);
  printf("WSC\n");

}


void ConnectionManager::clientInitAck(const QVariantMap &msg) {
  // Core has accepted our version info and sent its own. Let's see if we accept it as well...
  uint ver = msg["ProtocolVersion"].toUInt();
  if(ver < Quassel::buildInfo().clientNeedsProtocol) {
    emit connectionError(tr("<b>The Quassel Core you are trying to connect to is too old!</b><br>"
        "Need at least core/client protocol v%1 to connect.").arg(Quassel::buildInfo().clientNeedsProtocol));
    disconnectFromCore();
    return;
  }
  emit connectionMsg(msg["CoreInfo"].toString());

#ifndef QT_NO_COMPRESS
  if(msg["SupportsCompression"].toBool()) {
    socket->setProperty("UseCompression", true);
  }
#endif

  _coreMsgBuffer = msg;
#ifdef HAVE_SSL
  if(coreConnectionInfo["useSsl"].toBool()) {
    if(msg["SupportSsl"].toBool()) {
      QSslSocket *sslSocket = qobject_cast<QSslSocket *>(socket);
      Q_ASSERT(sslSocket);
      connect(sslSocket, SIGNAL(encrypted()), this, SLOT(sslSocketEncrypted()));
      connect(sslSocket, SIGNAL(sslErrors(const QList<QSslError> &)), this, SLOT(sslErrors(const QList<QSslError> &)));

      sslSocket->startClientEncryption();
    } else {
      emit connectionError(tr("<b>The Quassel Core you are trying to connect to does not support SSL!</b><br />If you want to connect anyways, disable the usage of SSL in the account settings."));
      disconnectFromCore();
    }
    return;
  }
#endif
  // if we use SSL we wait for the next step until every SSL warning has been cleared
  connectionReady();
}

void ConnectionManager::connectionReady() {
  if(!_coreMsgBuffer["Configured"].toBool()) {
    emit connectionError(tr("The core was not set up."
                            " Please connect with a desktop client and set up the core"));
    disconnectedFromCore();
    return;
  } else if(_coreMsgBuffer["LoginEnabled"].toBool()) {
    emit startLogin();
  }
  _coreMsgBuffer.clear();
  //disconnect(this, SIGNAL(handleIgnoreWarnings(bool)), this, 0);
}


void ConnectionManager::disconnectedFromCore(){
  emit socketDisconnected();
  resetConnection();
  // FIXME handle disconnects gracefully

}
void ConnectionManager::disconnectFromCore(){
    resetConnection();
}


void ConnectionManager::coreSocketError(QAbstractSocket::SocketError err){
}
void ConnectionManager::socketStateChanged(QAbstractSocket::SocketState state){

}
void ConnectionManager::coreHasData(){
  QVariant item;
  printf("Gotdata\n");
  while(SignalProxy::readDataFromDevice(socket, _blockSize, item)) {
    emit recvPartialItem(1,1);
    QVariantMap msg = item.toMap();
    if(!msg.contains("MsgType")) {
      // This core is way too old and does not even speak our init protocol...
      emit connectionError(tr("The Quassel Core you try to connect to is too old! Please consider upgrading."));
      disconnectFromCore();
      return;
    }
    if(msg["MsgType"] == "ClientInitAck") {
      clientInitAck(msg);
    } else if(msg["MsgType"] == "ClientInitReject") {
      emit connectionError(msg["Error"].toString());
      disconnectFromCore();
      return;
    } else if(msg["MsgType"] == "CoreSetupAck") {
      emit coreSetupSuccess();
    } else if(msg["MsgType"] == "CoreSetupReject") {
      emit coreSetupFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginReject") {
      emit loginFailed(msg["Error"].toString());
    } else if(msg["MsgType"] == "ClientLoginAck") {
      // prevent multiple signal connections
      //disconnect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));
      //connect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));
      emit loginSuccess();
    } else if(msg["MsgType"] == "SessionInit") {
      sessionStateReceived(msg["SessionState"].toMap());
      break; // this is definitively the last message we process here!
    } else {
      emit connectionError(tr("<b>Invalid data received from core!</b><br>Disconnecting."));
      disconnectFromCore();
      return;
    }
  }
  if(_blockSize > 0) {
    emit recvPartialItem(socket->bytesAvailable(), _blockSize);
  }
}
void ConnectionManager::sessionStateReceived(const QVariantMap &state) {
  //emit sessionProgress(1, 1);
  //disconnect(this, SIGNAL(recvPartialItem(quint32, quint32)), this, SIGNAL(sessionProgress(quint32, quint32)));

  // rest of communication happens through SignalProxy...
  disconnect(socket, 0, this, 0);
  // ... but we still want to be notified about errors...
  connect(socket, SIGNAL(disconnected()), this, SLOT(disconnectedFromCore()));
  connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(coreSocketError(QAbstractSocket::SocketError)));
  socket->setParent(0);
  signalProxy->addPeer(socket);
  //Client::instance()->setConnectedToCore(coreConnectionInfo["AccountId"].value<AccountId>(), _socket);
  syncToCore(state);
}
void ConnectionManager::syncToCore(const QVariantMap &sessionState) {
  // create identities
    printf("GSS\n");
    loggedIn=true;
    emit gotCoreSessionState(sessionState);
      //QVariantList networkids = sessionState["NetworkIds"].toList();

      // prepare sync progress thingys...
      // FIXME: Care about removal of networks
      //numNetsToSync = networkids.count();
      //emit networksProgress(0, numNetsToSync);



/*  foreach(QVariant vid, sessionState["Identities"].toList()) {
    Client::instance()->coreIdentityCreated(vid.value<Identity>());
  }

  // create buffers
  // FIXME: get rid of this crap
  QVariantList bufferinfos = sessionState["BufferInfos"].toList();
  NetworkModel *networkModel = Client::networkModel();
  Q_ASSERT(networkModel);
  foreach(QVariant vinfo, bufferinfos)
    networkModel->bufferUpdated(vinfo.value<BufferInfo>());  // create BufferItems

  QVariantList networkids = sessionState["NetworkIds"].toList();

  // prepare sync progress thingys...
  // FIXME: Care about removal of networks
  numNetsToSync = networkids.count();
  emit networksProgress(0, numNetsToSync);

  // create network objects
  foreach(QVariant networkid, networkids) {
    NetworkId netid = networkid.value<NetworkId>();
    if(Client::network(netid))
      continue;
    Network *net = new Network(netid, Client::instance());
    netsToSync.insert(net);
    connect(net, SIGNAL(initDone()), this, SLOT(networkInitDone()));
    Client::addNetwork(net);
  }*/
  checkSyncState();
}
void ConnectionManager::checkSyncState() {
  /*if(netsToSync.isEmpty()) {
    //Client::instance()->setSyncedToCore();
    emit syncFinished();
  }*/
}

SignalProxy*  ConnectionManager::getSignalProxy(){
    return signalProxy;
}
bool ConnectionManager::isLoggedIn(){
    return loggedIn;
}
