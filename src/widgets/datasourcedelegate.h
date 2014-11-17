/* This file is part of Zanshin

   Copyright 2014 Christian Mollekopf <mollekopf@kolabsys.com>
   Copyright 2014 Kevin Ottens <ervin@kde.org>

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


#ifndef DATASOURCEDELEGATE_H
#define DATASOURCEDELEGATE_H

#include <QStyledItemDelegate>

#include "domain/datasource.h"

namespace Widgets
{

class DataSourceDelegate : public QStyledItemDelegate
{
    Q_OBJECT
    Q_ENUMS(Action)
public:
    enum Action {
        AddToList = 0,
        RemoveFromList,
        Bookmark
    };

    explicit DataSourceDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:
    void actionTriggered(const Domain::DataSource::Ptr &source, int action);

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index);

private:
    QHash<Action, QPixmap> m_pixmaps;
};

}

#endif

