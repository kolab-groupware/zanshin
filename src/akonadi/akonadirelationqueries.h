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

#ifndef AKONADI_RELATIONQUERIES_H
#define AKONADI_RELATIONQUERIES_H

#include <Akonadi/Item>
#include <Akonadi/Relation>

#include "domain/relationqueries.h"
#include "domain/livequery.h"
#include "akonadistorageinterface.h"

namespace Akonadi {

class MonitorInterface;
class SerializerInterface;

class RelationQueries : public QObject, public Domain::RelationQueries
{
    Q_OBJECT
public:
    typedef Domain::LiveQuery<QPair<Akonadi::Item, Akonadi::Relation>, Domain::Relation::Ptr> RelationQuery;
    typedef Domain::QueryResult<Domain::Relation::Ptr> RelationResult;
    typedef Domain::QueryResultProvider<Domain::Relation::Ptr> RelationProvider;

    explicit RelationQueries(QObject *parent = 0);
    RelationQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor);
    virtual ~RelationQueries();

    RelationResult::Ptr findRelations(Domain::Artifact::Ptr) const;

private:
    StorageInterface *m_storage;
    SerializerInterface *m_serializer;
    bool m_ownInterfaces;
};

} // akonadi namespace

#endif // AKONADI_RELATIONQUERIES_H