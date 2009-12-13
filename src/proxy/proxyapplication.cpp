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

#include "proxyapplication.h"
#include "prototools.h"

#include <QStringList>
#include <QTcpServer>
#include <QtDebug>

#include "client.h"
#include "cliparser.h"
#include "proxyconnection.h"

#include <stdlib.h>
#include <time.h>

ProxyApplication::ProxyApplication(int &argc, char **argv)
  : QCoreApplication(argc, argv),
    Quassel(),
    _aboutToQuit(false)
{
    corePort=0;
    port=0;

  nextSid=0;
  setDataDirPaths(findDataDirPaths());
  setRunMode(Quassel::ClientOnly);
}

bool ProxyApplication::init() {
  if(Quassel::init()) {
    srand ( time(NULL) );
    // FIXME: MIGRATION 0.3 -> 0.4: Move database and core config to new location
    // Move settings, note this does not delete the old files
#ifdef Q_WS_MAC
    QSettings newSettings("quassel-irc.org", "quasselclient");
#else

# ifdef Q_WS_WIN
    QSettings::Format format = QSettings::IniFormat;
# else
    QSettings::Format format = QSettings::NativeFormat;
# endif

    QString newFilePath = Quassel::configDirPath() + "quasselclient"
    + ((format == QSettings::NativeFormat) ? QLatin1String(".conf") : QLatin1String(".ini"));
    QSettings newSettings(newFilePath, format);
#endif /* Q_WS_MAC */

    if(newSettings.value("Config/Version").toUInt() == 0) {
#     ifdef Q_WS_MAC
        QString org = "quassel-irc.org";
#     else
        QString org = "Quassel Project";
#     endif
      QSettings oldSettings(org, "Quassel Client");
      if(oldSettings.allKeys().count()) {
        qWarning() << "\n\n*** IMPORTANT: Config and data file locations have changed. Attempting to auto-migrate your client settings...";
        foreach(QString key, oldSettings.allKeys())
          newSettings.setValue(key, oldSettings.value(key));
        newSettings.setValue("Config/Version", 1);
        qWarning() << "*   Your client settings have been migrated to" << newSettings.fileName();
        qWarning() << "*** Migration completed.\n\n";
      }
    }
      coreHost=Quassel::optionValue("address");
      corePort=Quassel::optionValue("port").toInt();
      port = Quassel::optionValue("listen-port").toInt();
      listenAddress=Quassel::optionValue("listen");

    // MIGRATION end

    // check settings version
    // so far, we only have 1

    // session resume
    //Proxy *proxy = new Proxy();
    //proxy->init();

    if(!startListening()){
        return false;
    }
    printf("Inited\n");
    return true;
  }
  printf("Noinit\n");
  return false;
}

bool ProxyApplication::startListening() {//copyed from core.cpp
  server=new QTcpServer();
  server6=new QTcpServer();
  connect(server,SIGNAL(newConnection()),this,SLOT(newConnection()));
  connect(server6,SIGNAL(newConnection()),this,SLOT(newConnection()));

  bool success = false;

  const QStringList listen_list = listenAddress.split(",", QString::SkipEmptyParts);
  if(listen_list.size() > 0) {
    foreach (const QString listen_term, listen_list) {  // TODO: handle multiple interfaces for same TCP version gracefully
      QHostAddress addr;
      if(!addr.setAddress(listen_term)) {
        qCritical() << qPrintable(
          tr("Invalid listen address %1")
            .arg(listen_term)
        );
      } else {
        switch(addr.protocol()) {
          case QAbstractSocket::IPv4Protocol:
            if(server->listen(addr, port)) {
              /*qInfo() << qPrintable(
                tr("Listening for GUI clients on IPv4 %1 port %2 using protocol version %3")
                  .arg(addr.toString())
                  .arg(server->serverPort())                  .arg(Quassel::buildInfo().protocolVersion)
              );*/
              success = true;
            } else
              qWarning() << qPrintable(
                tr("Could not open IPv4 interface %1:%2: %3")
                  .arg(addr.toString())
                  .arg(port)
                  .arg(server->errorString()));
            break;
          case QAbstractSocket::IPv6Protocol:
            if(server6->listen(addr, port)) {
              /*qInfo() << qPrintable(
                tr("Listening for GUI clients on IPv6 %1 port %2 using protocol version %3")
                  .arg(addr.toString())
                  .arg(server->serverPort())
                  .arg(Quassel::buildInfo().protocolVersion)
              );*/
              success = true;
            } else {
              // if v4 succeeded on Any, the port will be already in use - don't display the error then
              // FIXME: handle this more sanely, make sure we can listen to both v4 and v6 by default!
              if(!success || server6->serverError() != QAbstractSocket::AddressInUseError)
                qWarning() << qPrintable(
                  tr("Could not open IPv6 interface %1:%2: %3")
                  .arg(addr.toString())
                  .arg(port)
                  .arg(server->errorString()));
            }
            break;
          default:
            qCritical() << qPrintable(
              tr("Invalid listen address %1, unknown network protocol")
                  .arg(listen_term)
            );
            break;
        }
      }
    }
  }
  if(!success)
    qCritical() << qPrintable(tr("Could not open any network interfaces to listen on!"));

  return success;
}

void ProxyApplication::newConnection (){
    if(server->hasPendingConnections()){
        QTcpSocket *nextsock;
        while((nextsock=server->nextPendingConnection())!=NULL){
            ProxyConnection *pc = new ProxyConnection(nextsock,this);
        }
    }
    if(server6->hasPendingConnections()){
        QTcpSocket *nextsock;
        while((nextsock=server6->nextPendingConnection())!=NULL){
            ProxyConnection *pc = new ProxyConnection(nextsock,this);
        }
    }

}

ProxyApplication::~ProxyApplication() {
}
void ProxyApplication::removeSession(ProxyUser *snd){
    printf("Removession %s\n",snd->getUsername().toUtf8().constData());
    proxyUsers.remove(snd->getUsername());
    snd->deleteLater();
}
void ProxyApplication::registerSession( ProxyUser *snd){
    printf("TryRegsession %s\n",snd->getUsername().toUtf8().constData());
    if(!proxyUsers.contains(snd->getUsername())){
        proxyUsers.insert(snd->getUsername(),snd);
        printf("Regsession %s\n",snd->getUsername().toUtf8().constData());
    }
}
ProxyUser *ProxyApplication::getSession(QString username){
    return proxyUsers.value(username);
}
int ProxyApplication::getCorePort(){
    return corePort;
}
QString ProxyApplication::getCoreHost(){
    return coreHost;
}
