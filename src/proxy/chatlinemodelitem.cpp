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

#include <QFontMetrics>
#include <QTextBoundaryFinder>

#include "chatlinemodelitem.h"
#include "chatlinemodel.h"
//#include "qtui.h"
//#include "qtuistyle.h"

// ****************************************
// the actual ChatLineModelItem
// ****************************************
ChatLineModelItem::ChatLineModelItem(const Message &msg)
  : MessageModelItem()
{
  this->msg=msg;
}

QVariant ChatLineModelItem::data(int column, int role) const {
  MessageModel::ColumnType col = (MessageModel::ColumnType)column;
  QVariant variant;
  switch(col) {
  case ChatLineModel::TimestampColumn:
    variant = timestampData(role);
    break;
  case ChatLineModel::SenderColumn:
    variant = senderData(role);
    break;
  case ChatLineModel::ContentsColumn:
    variant = contentsData(role);
    break;
  default:
    break;
  }
  if(!variant.isValid())
    return MessageModelItem::data(column, role);
  return variant;
}

QVariant ChatLineModelItem::timestampData(int role) const {
  switch(role) {
  case ChatLineModel::DisplayRole:
  case ChatLineModel::EditRole:
  case ChatLineModel::FormatRole:
    return msg.timestamp();
  }
  return QVariant();
}

QVariant ChatLineModelItem::senderData(int role) const {
  switch(role) {
  case ChatLineModel::DisplayRole:
  case ChatLineModel::EditRole:
  case ChatLineModel::FormatRole:
    return msg.sender();
  }
  return QVariant();
}

QVariant ChatLineModelItem::contentsData(int role) const {

  switch(role) {
  case ChatLineModel::DisplayRole:
  case ChatLineModel::EditRole:
  case ChatLineModel::FormatRole:
  case ChatLineModel::WrapListRole:
    return msg.contents();
  }
  return QVariant();
}


