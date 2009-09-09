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

#ifndef COREBUFFERSYNCER_H
#define COREBUFFERSYNCER_H

#include "buffersyncer.h"

class CoreSession;

class CoreBufferSyncer : public BufferSyncer {
  SYNCABLE_OBJECT
  Q_OBJECT

public:
  CoreBufferSyncer(CoreSession *parent);

public slots:
  virtual void requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId);

  virtual inline void requestRemoveBuffer(BufferId buffer) { removeBuffer(buffer); }
  virtual void removeBuffer(BufferId bufferId);

  virtual inline void requestRenameBuffer(BufferId buffer, QString newName) { renameBuffer(buffer, newName); }
  virtual void renameBuffer(BufferId buffer, QString newName);

  virtual inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) { mergeBuffersPermanently(buffer1, buffer2); }
  virtual void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2);

  virtual void requestPurgeBufferIds();

  void storeDirtyIds();

protected:
  virtual void customEvent(QEvent *event);

private:
  CoreSession *_coreSession;
  bool _purgeBuffers;

  QSet<BufferId> dirtyBuffers;

  void purgeBufferIds();
};

#endif //COREBUFFERSYNCER_H
