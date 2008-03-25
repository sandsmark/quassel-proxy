/***************************************************************************
 *   Copyright (C) 2005-08 by the Quassel IRC Team                         *
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

#ifndef BACKLOGMANAGER_H
#define BACKLOGMANAGER_H

#include "syncableobject.h"
#include "types.h"

class BacklogManager : public SyncableObject {
  Q_OBJECT

public:
  BacklogManager(QObject *parent = 0);

public slots:
  virtual QVariantList requestBacklog(BufferId bufferId, int lastMsgs = -1, int offset = -1);
  virtual void receiveBacklog(BufferId, int, int, QVariantList) {};

signals:
  void backlogRequested(BufferId, int, int);

};

#endif // BACKLOGMANAGER_H