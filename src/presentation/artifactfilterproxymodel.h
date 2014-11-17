/* This file is part of Zanshin

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


#ifndef PRESENTATION_ARTIFACTFILTERPROXYMODEL_H
#define PRESENTATION_ARTIFACTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace Presentation {

class ArtifactFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_ENUMS(SortType)
public:
    enum SortType {
        TitleSort = 0,
        DateSort,
        ProgressSort,
        StatusSort
    };

    explicit ArtifactFilterProxyModel(QObject *parent = 0);

    SortType sortType() const;
    void setSortType(SortType type);

    void setSortOrder(Qt::SortOrder order);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

private:
    SortType m_sortType;
};

}

#endif // PRESENTATION_ARTIFACTFILTERPROXYMODEL_H
