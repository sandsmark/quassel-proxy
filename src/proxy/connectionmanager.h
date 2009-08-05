#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QAbstractSocket>
#include <QVariantMap>
#include <QSet>
#include <QString>
#include <QPointer>
#include <QIODevice>
class SignalProxy;

class ConnectionManager : public QObject
{
    Q_OBJECT;
public:
    ConnectionManager();
    ~ConnectionManager();
    SignalProxy* getSignalProxy();
    bool isLoggedIn();
protected:
    void clientInitAck(const QVariantMap &msg);
    void sessionStateReceived(const QVariantMap &state);
    void checkSyncState();
    void connectionReady();
public slots:
    void resetConnection();
    void coreSocketError(QAbstractSocket::SocketError);
    void socketStateChanged(QAbstractSocket::SocketState);
    void loginToCore(QString username,QString password);
    void connectToCore(QVariantMap &);
    void disconnectFromCore();
protected slots:
    void coreHasData();
    void connectedToCore();
    void disconnectedFromCore();
    void syncToCore(const QVariantMap &sessionState);
signals:
    void startLogin();
    void connectionEstablished();
    void connectionError(QString err);
    void connectionMsg(QString msg);
    void connectionWarning(QString warn);
    void coreSetupSuccess();
    void syncFinished();
    void coreSetupFailed(QString msg);
    void loginFailed(QString);
    void loginSuccess();
    void recvPartialItem(quint32 avail, quint32 size);
    void socketDisconnected();
    void gotCoreSessionState(const QVariantMap &sessionState);
//    void syncronizeNetwork(const QVariant net);
private:
  QPointer<QIODevice> socket;
  quint32 _blockSize;

  QVariantMap coreConnectionInfo;
  QVariantMap _coreMsgBuffer;
  bool loggedIn;
  //QSet<QObject *> netsToSync;
  //int numNetsToSync;
  QPointer<SignalProxy>  signalProxy;

};

#endif // CONNECTIONMANAGER_H
