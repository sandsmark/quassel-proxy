/***************************************************************************
 *   Copyright (C) 2005-2012 by the Quassel Project                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef ABSTRACTUI_H
#define ABSTRACTUI_H

#include <QObject>
#include <QVariantMap>
//#include "message.h"

class MessageFilter;
class MessageModel;
class AbstractMessageProcessor;
class AbstractActionProvider;

class QAction;
class QMenu;

class AbstractUi : public QObject
{
    Q_OBJECT

public:
    AbstractUi(QObject *parent = 0) : QObject(parent) {}
    virtual ~AbstractUi() {}
    virtual void init() = 0; // called after the client is initialized
    virtual MessageModel *createMessageModel(QObject *parent) = 0;
    virtual AbstractMessageProcessor *createMessageProcessor(QObject *parent) = 0;

protected slots:
    virtual void connectedToCore() {}
    virtual void disconnectedFromCore() {}

signals:
    void connectToCore(const QVariantMap &connInfo);
    void disconnectFromCore();
};


#endif
