/* This file is part of Zanshin

   Copyright 2014 Kevin Ottens <ervin@kde.org>
   Copyright 2014 Franck Arrecot <franck.arrecot@gmail.com>

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


#ifndef PRESENTATION_TAGPAGEMODEL_H
#define PRESENTATION_TAGPAGEMODEL_H

#include "presentation/pagemodel.h"

#include "domain/tag.h"

namespace Domain
{
    class TagQueries;
}

namespace Presentation {

class TagPageModel : public PageModel
{
    Q_OBJECT
public:
    explicit TagPageModel(const Domain::Tag::Ptr &tag,
                          Domain::TagQueries *tagQueries,
                          Domain::TaskQueries *taskQueries,
                          Domain::TaskRepository *taskRepository,
                          Domain::NoteRepository *noteRepository,
                          QObject *parent = 0);

    Domain::Tag::Ptr tag() const;

    void addTask(const QString &title);
    void removeItem(const QModelIndex &index);

private:
    QAbstractItemModel *createCentralListModel();

    Domain::Tag::Ptr m_tag;
    Domain::TagQueries *m_tagQueries;
};

}

#endif // PRESENTATION_TAGPAGEMODEL_H
