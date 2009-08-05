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

#include <QStringList>
#include <QTcpServer>

#include "client.h"
#include "cliparser.h"
#include "proxy.h"


ProxyApplication::ProxyApplication(int &argc, char **argv)
  : QCoreApplication(argc, argv),
    Quassel(),
    _aboutToQuit(false)
{
  port=4342;
  nextSid=0;
  setDataDirPaths(findDataDirPaths());
  setRunMode(Quassel::ClientOnly);
}

bool ProxyApplication::init() {
  if(Quassel::init()) {

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

    // MIGRATION end

    // check settings version
    // so far, we only have 1

    // session resume
    //Proxy *proxy = new Proxy();
    //proxy->init();
    server=new QTcpServer();
    connect(server,SIGNAL(newConnection()),this,SLOT(newConnection()));
    server->listen(QHostAddress::Any,port);

    printf("Inited\n");
    return true;
  }
  printf("Noinit\n");
  return false;
}
void ProxyApplication::newConnection (){
    if(server->hasPendingConnections()){
        QTcpSocket *nextsock;
        while((nextsock=server->nextPendingConnection())!=NULL){
            Proxy *p = new Proxy(nextsock,nextSid);
            //p->setSid(nextSid);
            connect(p,SIGNAL(removeSession()),this,SLOT(removeSession()));
            connect(p,SIGNAL(switchSid(quasselproxy::Packet)),this,SLOT(switchSid(quasselproxy::Packet)));
            proxies.insert(nextSid,p);
            nextSid++;
        }
    }
}

ProxyApplication::~ProxyApplication() {
}
void ProxyApplication::removeSession(){
    Proxy *snd=qobject_cast<Proxy*>(sender());
    proxies.remove(snd->getSid());
    delete(snd);
}
void ProxyApplication::switchSid(quasselproxy::Packet pkg){
    Proxy *snd=qobject_cast<Proxy*>(sender());
    Proxy *stored;
    bool succeeded=false;
    //if(proxies.contains(sid))
    proxies.remove(snd->getSid());//remove the old proxy
    if(proxies.contains(pkg.setup().sessionid())){//resume session
        stored=proxies.value(pkg.setup().sessionid());
        if(!stored->hasClient()){
            if(stored->validateCreditals(pkg.setup())){//ok connect session
                stored->giveClientSocket(snd->takeClientSocket());
                printf("Takeover complete\n");
                succeeded=true;
            }else{
                //else wrong username/password
                printf("Wrongpsw\n");
            }
        }else{
            //else - session allready connected
            printf("Allready connected\n");
        }
    }else{//session not found
        printf("Session not found %d,%d\n",pkg.setup().sessionid(),proxies.count());
        foreach(int i,proxies.keys()){
            printf("Key:%d\n",i);
        }
    }
    if(!succeeded){
        printf("Takeover did not succeed\n");
        //disconnect
        quasselproxy::Packet spkg;
        spkg.mutable_statusinfo()->set_errormsg("Could not associate with the established session");
        snd->sendPacket(spkg);
        snd->getClientSocket()->close();
    }
    delete(snd);
}
