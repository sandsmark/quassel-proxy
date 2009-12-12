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

#ifndef PROXYMESSAGEPROCESSOR_H_
#define PROXYMESSAGEPROCESSOR_H_

#include "abstractmessageprocessor.h"
#include "message.h"
#include "networkmodel.h"
class ProxyUser;

class ProxyMessageProcessor : public AbstractMessageProcessor {
  Q_OBJECT

public:
  ProxyMessageProcessor(QObject *parent,ProxyUser *proxy);
  virtual void reset();

public slots:
  virtual void process(Message &msg);
  virtual void process(QList<Message> &msgs);
protected:
  ProxyUser *proxy;
};

#endif
