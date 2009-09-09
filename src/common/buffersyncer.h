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

#ifndef BUFFERSYNCER_H_
#define BUFFERSYNCER_H_

#include "syncableobject.h"
#include "types.h"

class BufferSyncer : public SyncableObject {
  SYNCABLE_OBJECT
  Q_OBJECT

public:
  explicit BufferSyncer(QObject *parent);
  explicit BufferSyncer(const QHash<BufferId, MsgId> &lastSeenMsg, QObject *parent);

  inline virtual const QMetaObject *syncMetaObject() const { return &staticMetaObject; }

  MsgId lastSeenMsg(BufferId buffer) const;

public slots:
  QVariantList initLastSeenMsg() const;
  void initSetLastSeenMsg(const QVariantList &);

  virtual inline void requestSetLastSeenMsg(BufferId buffer, const MsgId &msgId) { REQUEST(ARG(buffer), ARG(msgId)) }

  virtual inline void requestRemoveBuffer(BufferId buffer) { REQUEST(ARG(buffer)) }
  virtual void removeBuffer(BufferId buffer);

  virtual inline void requestRenameBuffer(BufferId buffer, QString newName) { REQUEST(ARG(buffer), ARG(newName)) }
  virtual inline void renameBuffer(BufferId buffer, QString newName) { SYNC(ARG(buffer), ARG(newName)) emit bufferRenamed(buffer, newName); }

  virtual inline void requestMergeBuffersPermanently(BufferId buffer1, BufferId buffer2) { emit REQUEST(ARG(buffer1), ARG(buffer2)) }
  virtual void mergeBuffersPermanently(BufferId buffer1, BufferId buffer2);

  virtual inline void requestPurgeBufferIds() { REQUEST(NO_ARG); }

signals:
  void lastSeenMsgSet(BufferId buffer, const MsgId &msgId);
  void bufferRemoved(BufferId buffer);
  void bufferRenamed(BufferId buffer, QString newName);
  void buffersPermanentlyMerged(BufferId buffer1, BufferId buffer2);

protected slots:
  bool setLastSeenMsg(BufferId buffer, const MsgId &msgId);
  QList<BufferId> bufferIds() const { return _lastSeenMsg.keys(); }

private:
  QHash<BufferId, MsgId> _lastSeenMsg;
};

#endif
