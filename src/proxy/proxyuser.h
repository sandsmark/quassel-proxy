/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) version 3.                                           *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef QPROXYUSER_H
#define QPROXYUSER_H

#include "abstractui.h"
#include "bufferinfo.h"
#include "identity.h"
#ifdef MAIN_CPP
#include "proxy/protocol.pb.h"
#else
#include "protocol.pb.h"
#endif
#include "channelinfo.h"
//#include "mainwin.h"


class ConnectionManager;
class Identity;
class Network;
class IrcUser;
class NetworkInfo;
class BacklogRequester;
class QTcpSocket;
class IrcChannel;
class Message;
class ProxyApplication;
class ProxyConnection;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class ProxyUser : public AbstractUi {
  Q_OBJECT

public:
  ProxyUser(ProxyApplication *app);
  ~ProxyUser();

  MessageModel *createMessageModel(QObject *parent);
  AbstractMessageProcessor *createMessageProcessor(QObject *parent);
  void setSid(int sid);
  int getSid();
  QString getUsername();

  bool validateCreditals(quasselproxy::SessionSetup setup);
  bool setCreditals(quasselproxy::SessionSetup setup);
  bool hasClient();
  void registerConnection(ProxyConnection* conn);
  void deregisterConnection(ProxyConnection* conn);
  QPointer<BacklogRequester> getBacklogRequester();

  QHash<quint32, Identity *>* getIdentities();
  QHash<quint32, BufferInfo>* getBufferInfos();
  QHash<quint32, Network* >* getNetworks();
  BufferViewManager *getBufferViewManager();
  BufferSyncer* getBufferSyncer();
  bool isLoggedIn();
  bool isForUs(const Message& msg);
public slots:
  virtual void init();
signals:
    //signals connected to the signalproxy
    void sendInput(BufferInfo,QString);
    void requestNetworkStates();

    void requestCreateIdentity(const Identity &, const QVariantMap &);
    void requestRemoveIdentity(IdentityId);

    void requestCreateNetwork(const NetworkInfo &, const QStringList &);
    void requestRemoveNetwork(NetworkId);

    //signals for relaying signal proxy stuff to connections
    void recvMessage(const Message&);
    void backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList);
    void bufferUpdated(BufferInfo);

    //signals for updating connections
    void updatePersistentInfo();
    //void removeSession();
    //void switchSession(quasselproxy::Packet);
    //void registerSession(quasselproxy::Packet);
    //end signals connected to the signalproxy

protected slots:
    //start slots connected to SignalProxy
    void ircUsersJoined(QStringList nicks, QStringList modes);
    void ircUserParted(IrcUser *ircuser);
    void ircUserNickSet(IrcUser *ircuser, QString nick);
    void ircUserModeAdded(IrcUser *ircuser, QString mode);
    void ircUserModeRemoved(IrcUser *ircuser, QString mode);
    void ircUserModesSet(IrcUser *ircuser, QString modes);


    void coreIdentityCreated(const Identity &);
    void coreIdentityRemoved(IdentityId);

    void coreNetworkCreated(NetworkId id);
    void coreNetworkRemoved(NetworkId id);
    //void packetRecievedFromClient(quasselproxy::Packet pkg);
    //void updateActivity();
    //end slots connected to SignalProxy
  void identityRecieved(const IdentityId &);
  void startLogin();
  void loginFailed(const QString &msg);

  //void startCoreConfig(const QVariantList &);
  void gotCoreSessionState(const QVariantMap&);
  void netsyncComplete();
  void syncComplete();
  void disconnectedFromCore();

  //void clientHasData();
  //void activateBuffer(quint32 newbid);
  void disconnect();
  //void checkActivity();
  //void socketDisconnected();
//  void syncronizeNetwork(const QVariant net);
private slots:
  void bufferUpdatedPrivateSlot(BufferInfo);
private:
  void bufferUpdatedPrivate(BufferInfo,bool);
  void updatePersistentInfoPrivate();
  //static QPointer<Proxy> _instance;
  //ClientSyncer *syncer;
  //Client *client;
  ConnectionManager *conn;
  //QTcpSocket *client;
  QHash<quint32, Identity *> identities;
  QHash<quint32, BufferInfo> bufferInfos;
  QHash<quint32,Network * > networks;
/*
  QHash<quint32,QList<quint32> > channelClientUsers;
  QHash<quint32,quasselproxy::IrcChannel > channelClientInfo;
  QHash<quint32,quint32> bufferActivivty;
*/
  QSet<QObject *> netsToSync;
  QPointer<BacklogRequester> back;

  bool syncronizing;
  //bool clientDisconnected;
  bool authenticatedWithCore;
  //quasselproxy::SessionSetup setup;
  QString username;
  QString password;
  int clientPersistentInfoVersion;
  ProxyApplication *app;
  QList<QPointer<ProxyConnection> > clientConenections;
  BufferSyncer *bufsync;
  BufferViewManager* bufViewMan;
  //QList<QPointer<BufferViewConfig> > bufViewConfigs;
  bool bufferViewManSynchronized;
  SignalProxy *p;
};

//Proxy *Proxy::instance() { return _instance ? _instance.data() : new Proxy(); }

#endif
