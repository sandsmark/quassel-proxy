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

#include <cstdlib>
#define MAIN_CPP

#ifdef BUILD_CORE
#  include "coreapplication.h"
#elif defined BUILD_QTUI
#  include "qtuiapplication.h"
#elif defined BUILD_PROXY
#  include "proxyapplication.h"
#elif defined BUILD_MONO
#  include "monoapplication.h"

#else
#error "Something is wrong - you need to #define a build mode!"
#endif

// We don't want quasselcore to depend on KDE
#if defined HAVE_KDE && defined BUILD_CORE
#  undef HAVE_KDE
#endif

#ifdef HAVE_KDE
#  include <KAboutData>
#  include "kcmdlinewrapper.h"
#endif

#include "cliparser.h"
#include "quassel.h"

int main(int argc, char **argv) {

  // Setup build information and version string
  # include "version.gen"
  buildinfo.append(QString(",%1,%2").arg(__DATE__, __TIME__));
  Quassel::setupBuildInfo(buildinfo);
  QCoreApplication::setApplicationName(Quassel::buildInfo().applicationName);
  QCoreApplication::setOrganizationName(Quassel::buildInfo().organizationName);
  QCoreApplication::setOrganizationDomain(Quassel::buildInfo().organizationDomain);

  AbstractCliParser *cliParser;

#ifdef HAVE_KDE
  // We need to init KCmdLineArgs first
  // TODO: build an AboutData compat class to replace our aboutDlg strings
  KAboutData aboutData("quassel", "kdelibs4", ki18n("Quassel IRC"), Quassel::buildInfo().plainVersionString.toUtf8(),
                        ki18n("A modern, distributed IRC client"));
  aboutData.addLicense(KAboutData::License_GPL_V2);
  aboutData.addLicense(KAboutData::License_GPL_V3);
  aboutData.setBugAddress("http://bugs.quassel-irc.org/projects/quassel-irc/issues/new");
  aboutData.setOrganizationDomain(Quassel::buildInfo().organizationDomain.toUtf8());
  KCmdLineArgs::init(argc, argv, &aboutData);

  cliParser = new KCmdLineWrapper();
#else
  cliParser = new CliParser();
#endif
  Quassel::setCliParser(cliParser);

  // Initialize CLI arguments
  // NOTE: We can't use tr() at this point, since app is not yet created

  // put shared client&core arguments here
  cliParser->addSwitch("debug",'d', "Enable debug output");
  cliParser->addSwitch("help",'h', "Display this help and exit");

#ifndef BUILD_CORE
  // put client-only arguments here
  cliParser->addOption("qss <file.qss>", 0, "Load a custom application stylesheet");
  cliParser->addSwitch("debugbufferswitches", 0, "Enables debugging for bufferswitches");
  cliParser->addSwitch("debugmodel", 0, "Enables debugging for models");
#endif
#ifndef BUILD_QTUI
  // put core-only arguments here
  cliParser->addOption("listen <address>[,<address[,...]]>", 0, "The address(es) quasselcore will listen on", "0.0.0.0,::");
  cliParser->addOption("port <port>",'p', "The port quasselcore will listen at", QString("4242"));
  cliParser->addSwitch("norestore", 'n', "Don't restore last core's state");
  cliParser->addOption("logfile <path>", 'l', "Path to logfile");
  cliParser->addOption("loglevel <level>", 'L', "Loglevel Debug|Info|Warning|Error", "Info");
  cliParser->addOption("select-backend <backendidentifier>", 0, "Starts an interactive session and switches your current storage backend to the new one. Attempts a merge if the new backend is uninitialized and the old backend supports migration. Otherwise prompts for new user credentials!");
  cliParser->addSwitch("add-user", 0, "Starts an interactive session to add a new core user");
  cliParser->addOption("change-userpass <username>", 0, "Starts an interactive session to change the password of the user identified by username");
#endif

#ifdef HAVE_KDE
  // the KDE version needs this extra call to parse argc/argv before app is instantiated
  if(!cliParser->init()) {
    cliParser->usage();
    return EXIT_FAILURE;
  }
#endif

#  if defined BUILD_CORE
    CoreApplication app(argc, argv);
#  elif defined BUILD_QTUI
    QtUiApplication app(argc, argv);
#  elif defined BUILD_MONO
    MonolithicApplication app(argc, argv);
#  elif defined BUILD_PROXY
    ProxyApplication app(argc, argv);
#  endif

#ifndef HAVE_KDE
  // the non-KDE version parses after app has been instantiated
  if(!cliParser->init(app.arguments())) {
    cliParser->usage();
    return false;
  }
#endif

  if(!app.init()) return EXIT_FAILURE;
  return app.exec();
}
