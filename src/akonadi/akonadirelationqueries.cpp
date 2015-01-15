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

#include "akonadirelationqueries.h"

#include "akonadicollectionfetchjobinterface.h"
#include "akonadiitemfetchjobinterface.h"
#include "akonaditagfetchjobinterface.h"
#include "akonadirelationfetchjobinterface.h"

#include "akonadiserializer.h"
#include "akonadistorage.h"

#include "utils/jobhandler.h"

#include <QPointer>

using namespace Akonadi;

RelationQueries::RelationQueries(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_ownInterfaces(true)
{
}

RelationQueries::RelationQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_ownInterfaces(false)
{
}

RelationQueries::~RelationQueries()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
    }
}

RelationQueries::RelationResult::Ptr RelationQueries::findRelations(Domain::Artifact::Ptr artifact) const
{
    auto query =  RelationQueries::RelationQuery::Ptr::create();
    auto item = m_serializer->createItemFromArtifact(artifact);
    qDebug() << "looking for relations " << item.url();

    query->setFetchFunction([this, item, query] (const RelationQuery::AddFunction &add) {
        RelationFetchJobInterface *job = m_storage->fetchRelations(item);
        Utils::JobHandler::install(job->kjob(), [this, job, add, query] {
            //Hack to keep query alive
            Q_ASSERT(query);
            qDebug() << "Found relations " << job->relations().size();
            for (QPair<Item, Relation> rel : job->relations())
                add(rel);
        });
    });

    query->setConvertFunction([this] (const QPair<Item, Relation> &relatedItem) {
        return m_serializer->createRelationFromAkonadiRelation(relatedItem);
    });

    query->setUpdateFunction([this] (const QPair<Item, Relation> &akonadiRelation, Domain::Relation::Ptr &relation) {
        // m_serializer->updateRelationFromAkonadiRelation(relation, akonadiRelation);
    });
    query->setPredicateFunction([this] (const QPair<Item, Relation> &akonadiRelation) {
        return true;
    });
    query->setRepresentsFunction([this] (const QPair<Item, Relation> &akonadiRelation, const Domain::Relation::Ptr &relation) {
        return m_serializer->representsAkonadiRelation(relation, akonadiRelation);
    });

    return query->result();
}

