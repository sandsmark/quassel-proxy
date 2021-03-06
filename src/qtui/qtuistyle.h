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

#ifndef QTUISTYLE_H_
#define QTUISTYLE_H_

#include "uistyle.h"
#include "qtuisettings.h"

class QtUiStyle : public UiStyle
{
    Q_OBJECT

public:
    QtUiStyle(QObject *parent = 0);
    virtual ~QtUiStyle();

    virtual inline qreal firstColumnSeparator() const { return 6; }
    virtual inline qreal secondColumnSeparator() const { return 6; }

public slots:
    void generateSettingsQss() const;

private slots:
    void updateTimestampFormatString();

private:
    QString fontDescription(const QFont &font) const;
    QString color(const QString &key, UiSettings &settings) const;

    QString msgTypeQss(const QString &msgType, const QString &key, UiSettings &settings) const;
    QString senderQss(int i, UiSettings &settings) const;
    QString chatListItemQss(const QString &state, const QString &key, UiSettings &settings) const;
};


#endif
