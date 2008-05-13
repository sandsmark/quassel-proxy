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

#ifndef MESSAGEMODEL_H_
#define MESSAGEMODEL_H_

#include <QAbstractItemModel>
#include <QDateTime>

#include "message.h"
#include "types.h"

class MessageModelItem;
class MsgId;

class MessageModel : public QAbstractItemModel {
  Q_OBJECT

  public:
    enum MessageRole {
      MsgIdRole = Qt::UserRole,
      BufferIdRole,
      TypeRole,
      FlagsRole,
      TimestampRole,
      DisplayRole,
      FormatRole,
      UserRole
    };

    enum ColumnType {
      TimestampColumn, SenderColumn, TextColumn, UserColumnType
    };

    MessageModel(QObject *parent);
    virtual ~MessageModel();

    inline QModelIndex index(int row, int column, const QModelIndex &/*parent*/ = QModelIndex()) const { return createIndex(row, column); }
    inline QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
    inline int rowCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return _messageList.count(); }
    inline int columnCount(const QModelIndex &/*parent*/ = QModelIndex()) const { return 3; }

    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

    //virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    void insertMessage(const Message &);
    void insertMessages(const QList<Message> &);

  protected:
    virtual MessageModelItem *createMessageModelItem(const Message &) = 0;

  private:
    QList<MessageModelItem *> _messageList;

    int indexForId(MsgId);

};

class MessageModelItem {

  public:

    //! Creates a MessageModelItem from a Message object.
    /** This baseclass implementation takes care of all Message data *except* the stylable strings.
     *  Subclasses need to provide Qt::DisplayRole at least, which should describe the plaintext
     *  strings without formattings (e.g. for searching purposes).
     */
    MessageModelItem(const Message &);
    virtual ~MessageModelItem();

    virtual QVariant data(int column, int role) const;
    virtual bool setData(int column, const QVariant &value, int role) = 0;

  private:
    QDateTime _timestamp;
    MsgId _msgId;
    BufferId _bufferId;
    Message::Type _type;
    Message::Flags _flags;
};

#endif