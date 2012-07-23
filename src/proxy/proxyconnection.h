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

#ifndef QPROXYCONNECTION_H
#define QPROXYCONNECTION_H

#include "abstractui.h"
#include "bufferinfo.h"
#include "identity.h"
#include "bufferinfo.h"
#ifdef MAIN_CPP
#include "proxy/protocol.pb.h"
#else
#include "protocol.pb.h"
#endif
#include "channelinfo.h"
//#include "mainwin.h"

#include <QtCore/QDateTime>


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
class ProxyUser;
class BufferViewConfig;

//! This class encapsulates Quassel's Qt-based GUI.
/** This is basically a wrapper around MainWin, which is necessary because we cannot derive MainWin
 *  from both QMainWindow and AbstractUi (because of multiple inheritance of QObject).
 */
class ProxyConnection : public QObject {
  Q_OBJECT

public:
  ProxyConnection(QTcpSocket *client,ProxyApplication *app);
  ~ProxyConnection();

  void connectUserSignals();

  //MessageModel *createMessageModel(QObject *parent);
  //AbstractMessageProcessor *createMessageProcessor(QObject *parent);
  void setSid(int sid);
  int getSid();
  QString getUsername();
  void sendPacket(quasselproxy::Packet pkg);

  void convertMessage(const Message *msg,quasselproxy::Message *proto);
  bool validateCreditals(quasselproxy::SessionSetup setup);
  bool hasClient();
  QTcpSocket *getClientSocket();
  QTcpSocket *takeClientSocket();
  void giveClientSocket(QTcpSocket *sock);
  //inline static Proxy *instance();



public slots:
  //virtual void init();
  void syncComplete();
  void loginFailed();

#ifdef BUFVIEW
  void bufferViewManagerCreated();
  void bufferAdded(BufferId,int);
  void bufferRemoved(BufferId);*/
#endif
signals:
    //signals connected to the signalproxy
    void sendInput(BufferInfo,QString);
    void requestNetworkStates();
    void lastSeenMsg(BufferId, MsgId);

    void requestCreateIdentity(const Identity &, const QVariantMap &);
    void requestRemoveIdentity(IdentityId);

    void requestCreateNetwork(const NetworkInfo &, const QStringList &);
    void requestRemoveNetwork(NetworkId);

    //void removeSession();
    //void switchSession(quasselproxy::Packet);
    //void registerSession(quasselproxy::Packet);
    //end signals connected to the signalproxy

protected slots:
    //start slots connected to SignalProxy
    void recvMessage(const Message&);
    void bufferUpdated(BufferInfo);
    void updatePersistentInfo();

    void authenticateClient(quasselproxy::Packet pkg);
    void packetRecievedFromClient(quasselproxy::Packet pkg);
    void updateActivity();
    //end slots connected to SignalProxy
  //void identityRecieved(const IdentityId &);

  //void startCoreConfig(const QVariantList &);
  //void gotCoreSessionState(const QVariantMap&);
  //void netsyncComplete();

  void disconnectedFromCore();
  void backlogRecieved(BufferId, MsgId, MsgId, int, int, QVariantList);

  void clientHasData();
  void activateBuffer(quint32 newbid,quint32 lastSeenMsg=-1);
  void disconnect();
  void checkActivity();
//  void syncronizeNetwork(const QVariant net);
private:
  void generatePersistentInfo(quasselproxy::Packet *resp);
  void generateActivateBufferPacket(quint32 newbid,quasselproxy::Packet *resp,quint32 lastSeenMsg=-1);
  //static QPointer<Proxy> _instance;
  //ClientSyncer *syncer;
  //Client *client;
  ProxyUser *conn;
  QTcpSocket *client;
  ProxyApplication *app;
  BufferViewConfig *bufViewCfg;


  //QList<quint32> visitedBuffers;
  QHash<quint32, BufferInfo::Activity> bufferStates;
  //QHash<quint32, Identity *> identities;
  int clientPersistentInfoVersion;

  //Client/protobuf io variables
  QByteArray readBuf;
  quint32 bytesRead;
  bool newRead;
  bool clientPause;
  bool syncronizing;
  bool clientDisconnected;
  bool authenticatedWithCore;

  quint32 activeBuffer;
  quint32 lastSendtMessage;


  quint64 timebase;
  QTimer activityChecker;
  QDateTime lastActivity;
  quasselproxy::SessionSetup setup;

};

//Proxy *Proxy::instance() { return _instance ? _instance.data() : new Proxy(); }

#endif
