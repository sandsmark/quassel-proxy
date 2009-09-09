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

#ifndef QTUIAPPLICATION_H_
#define QTUIAPPLICATION_H_

#ifdef HAVE_KDE
#  include <KApplication>
#else
#  include <QApplication>
#endif

#include <QSessionManager>

#include "quassel.h"
#include "uisettings.h"

class QtUi;

#ifdef HAVE_KDE
class QtUiApplication : public KApplication, public Quassel {
#else
class QtUiApplication : public QApplication, public Quassel {
#endif

  Q_OBJECT

public:
  QtUiApplication(int &, char **);
  ~QtUiApplication();
  virtual bool init();

  void resumeSessionIfPossible();
  virtual void commitData(QSessionManager &manager);
  virtual void saveState(QSessionManager &manager);

  inline bool isAboutToQuit() const { return _aboutToQuit; }

protected:
  virtual void quit();

private:
  bool _aboutToQuit;
};

#endif
