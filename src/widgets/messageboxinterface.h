/* This file is part of Zanshin

   Copyright 2014 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/


#ifndef WIDGETS_MESSAGEBOXINTERFACE_H
#define WIDGETS_MESSAGEBOXINTERFACE_H

#include <QString>
#include <QSharedPointer>
#include <QMessageBox>

class QWidget;

namespace Widgets {

class MessageBoxInterface
{
public:
    typedef QSharedPointer<MessageBoxInterface> Ptr;

    virtual ~MessageBoxInterface();

    virtual QMessageBox::Button askConfirmation(QWidget *parent, const QString &title, const QString &text) = 0;
};

}

#endif
