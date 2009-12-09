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

#ifndef CHATLINEMODELITEM_H_
#define CHATLINEMODELITEM_H_

#include "messagemodel.h"

//#include "uistyle.h"

class ChatLineModelItem : public MessageModelItem {
public:
  ChatLineModelItem(const Message &);

  virtual QVariant data(int column, int role) const;

  virtual inline const Message &message() const { return msg; }
  virtual inline const QDateTime &timestamp() const { return msg.timestamp(); }
  virtual inline const MsgId &msgId() const { return msg.msgId(); }
  virtual inline const BufferId &bufferId() const { return msg.bufferId(); }
  virtual inline void setBufferId(BufferId bufferId) { msg.setBufferId(bufferId); }
  virtual inline Message::Type msgType() const { return msg.type(); }
  virtual inline Message::Flags msgFlags() const { return msg.flags(); }
  inline bool isTransfered() const { return transfered; }
  inline void setTransfered(bool t){ transfered=t; }
private:
   virtual QVariant timestampData(int role) const;
   virtual QVariant senderData(int role) const;
   virtual QVariant contentsData(int role) const;

  static unsigned char *TextBoundaryFinderBuffer;
  static int TextBoundaryFinderBufferSize;
  Message msg;
  bool transfered;
};

#endif
