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


#include "akonadidatasourcequeries.h"

#include "akonadicollectionfetchjobinterface.h"
#include "akonadicollectionsearchjobinterface.h"
#include "akonadiitemfetchjobinterface.h"
#include "akonadimonitorimpl.h"
#include "akonadiserializer.h"
#include "akonadistorage.h"

#include "utils/jobhandler.h"

using namespace Akonadi;

AkonadiDataSourceCache::AkonadiDataSourceCache(StorageInterface *storage)
    : QObject(),
    m_populated(false),
    m_storage(storage)
{

}

void AkonadiDataSourceCache::populate()
{
    CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive, StorageInterface::Tasks | StorageInterface::Notes);
    Utils::JobHandler::install(job->kjob(), [this, job] {
        if (job->kjob()->error()) {
            kWarning() << "Failed to fetch collections " << job->kjob()->errorString();
        }
        for (auto collection : job->collections()) {
            kDebug() << collection.id() << collection.parentCollection().id() << collection.name();

            auto topLevel = collection;
            while (topLevel.isValid() && topLevel != Collection::root()) {
                add(topLevel);
                topLevel = topLevel.parentCollection();
            }
        }
        m_populated = true;
        emit populated();
    });
}

void AkonadiDataSourceCache::add(const Collection &col)
{
    Collection::List &collections = m_collections[col.parentCollection().id()];
    if (!collections.contains(col)) {
        Q_ASSERT(col.id() != col.parentCollection().id());
        collections.append(col);
    }
}

void AkonadiDataSourceCache::remove(const Collection &col)
{
    m_collections[col.parentCollection().id()].removeAll(col);
    m_collections.remove(col.id());
}

void AkonadiDataSourceCache::update(const Collection & col)
{
    Collection::List &collections = m_collections[col.parentCollection().id()];
    collections.removeAll(col);
    collections.append(col);
}


CacheRetrievalJob::CacheRetrievalJob(AkonadiDataSourceCache *cache)
    : m_cache(cache)
{
}

void CacheRetrievalJob::start()
{
    if (m_cache->m_populated) {
        emitResult();
        return;
    }
    m_cache->populate();
    connect(m_cache, SIGNAL(populated()), this, SLOT(onPopulated()));
}

void CacheRetrievalJob::onPopulated()
{
    emitResult();
}

DataSourceQueries::DataSourceQueries(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_monitor(new MonitorImpl),
      m_ownInterfaces(true),
      m_dataSourceCache(new AkonadiDataSourceCache(m_storage))
{
    connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onCollectionAdded(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onCollectionRemoved(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onCollectionChanged(Akonadi::Collection)));
}

DataSourceQueries::DataSourceQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_monitor(monitor),
      m_ownInterfaces(false)
{
    connect(monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onCollectionAdded(Akonadi::Collection)));
    connect(monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onCollectionRemoved(Akonadi::Collection)));
    connect(monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onCollectionChanged(Akonadi::Collection)));
}

DataSourceQueries::~DataSourceQueries()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
        delete m_monitor;
    }
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findTasks() const
{
    if (!m_findTasks) {
        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            self->m_findTasks = self->createDataSourceQuery();
        }

        m_findTasks->setFetchFunction([this] (const DataSourceQuery::AddFunction &add) {
            CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive, StorageInterface::Tasks);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                for (auto collection : job->collections()) {
                    add(collection);
                }
            });
        });

        m_findTasks->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::FullPath);
        });
        m_findTasks->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::FullPath);
        });
        m_findTasks->setPredicateFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->isTaskCollection(collection);
        });
        m_findTasks->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findTasks->result();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findNotes() const
{
    if (!m_findNotes) {
        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            self->m_findNotes = self->createDataSourceQuery();
        }

        m_findNotes->setFetchFunction([this] (const DataSourceQuery::AddFunction &add) {
            CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive, StorageInterface::Notes);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                for (auto collection : job->collections())
                    add(collection);
            });
        });

        m_findNotes->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::FullPath);
        });
        m_findNotes->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::FullPath);
        });
        m_findNotes->setPredicateFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->isNoteCollection(collection);
        });
        m_findNotes->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findNotes->result();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findTopLevel() const
{
    if (!m_findTopLevel) {
        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            self->m_findTopLevel = self->createDataSourceQuery();
        }

        m_findTopLevel->setFetchFunction([this] (const DataSourceQuery::AddFunction &add) {
            CacheRetrievalJob *job = new CacheRetrievalJob(m_dataSourceCache.data());
            Utils::JobHandler::install(job, [this, job, add] {
                if (job->error()) {
                    kWarning() << job->errorString();
                    return;
                }

                foreach (const auto &collection, m_dataSourceCache->m_collections.value(Collection::root().id())) {
                    add(collection);
                }
            });
        });

        m_findTopLevel->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
        });
        m_findTopLevel->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });
        m_findTopLevel->setPredicateFunction([this] (const Akonadi::Collection &collection) {
            return collection.isValid()
                && collection.parentCollection() == Akonadi::Collection::root()
                && m_serializer->isListedCollection(collection);
        });
        m_findTopLevel->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findTopLevel->result();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findChildren(Domain::DataSource::Ptr source) const
{
    Collection root = m_serializer->createCollectionFromDataSource(source);
    if (!m_findChildren.contains(root.id())) {
        DataSourceQuery::Ptr query;

        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            query = self->createDataSourceQuery();
            self->m_findChildren.insert(root.id(), query);
        }

        query->setFetchFunction([this, root] (const DataSourceQuery::AddFunction &add) {
            CacheRetrievalJob *job = new CacheRetrievalJob(m_dataSourceCache.data());
            Utils::JobHandler::install(job, [this, root, job, add] {
                if (job->error()) {
                    kWarning() << job->errorString();
                    return;
                }

                foreach (const auto &collection, m_dataSourceCache->m_collections.value(root.id())) {
                    Q_ASSERT(collection.id() != root.id());
                    add(collection);
                }
            });
        });

        query->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
        });
        query->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });
        query->setPredicateFunction([this, root] (const Akonadi::Collection &collection) {
            return m_dataSourceCache->m_collections[root.id()].contains(collection);
        });
        query->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findChildren.value(root.id())->result();
}

QString DataSourceQueries::searchTerm() const
{
    return m_searchTerm;
}

void DataSourceQueries::setSearchTerm(QString term)
{
    if (m_searchTerm == term)
        return;

    m_searchTerm = term;
    if (m_findSearchTopLevel) {
        m_findSearchTopLevel->reset();
    }
    foreach(auto query, m_findSearchChildren.values())
        query->reset();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findSearchTopLevel() const
{
    if (!m_findSearchTopLevel) {
        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            self->m_findSearchTopLevel = self->createDataSourceQuery();
        }

        m_findSearchTopLevel->setFetchFunction([this] (const DataSourceQuery::AddFunction &add) {
            if (m_searchTerm.isEmpty())
                return;

            CollectionSearchJobInterface *job = m_storage->searchCollections(m_searchTerm);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                if (job->kjob()->error())
                    return;

                QHash<Collection::Id, Collection> topLevels;
                foreach (const auto &collection, job->collections()) {
                    auto topLevel = collection;
                    while (topLevel.parentCollection() != Collection::root())
                        topLevel = topLevel.parentCollection();
                    if (!topLevels.contains(topLevel.id()))
                        topLevels[topLevel.id()] = topLevel;
                }

                foreach (const auto &topLevel, topLevels.values())
                    add(topLevel);

            });
        });

        m_findSearchTopLevel->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
        });
        m_findSearchTopLevel->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });
        m_findSearchTopLevel->setPredicateFunction([this] (const Akonadi::Collection &collection) {
            return collection.isValid()
                && collection.parentCollection() == Akonadi::Collection::root();
        });
        m_findSearchTopLevel->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findSearchTopLevel->result();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findSearchChildren(Domain::DataSource::Ptr source) const
{
    Collection root = m_serializer->createCollectionFromDataSource(source);
    if (!m_findSearchChildren.contains(root.id())) {
        DataSourceQuery::Ptr query;

        {
            DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
            query = self->createDataSourceQuery();
            self->m_findSearchChildren.insert(root.id(), query);
        }

        query->setFetchFunction([this, root] (const DataSourceQuery::AddFunction &add) {
            if (m_searchTerm.isEmpty())
                return;

            CollectionSearchJobInterface *job = m_storage->searchCollections(m_searchTerm);
            Utils::JobHandler::install(job->kjob(), [this, root, job, add] {
                if (job->kjob()->error())
                    return;

                QHash<Collection::Id, Collection> children;
                foreach (const auto &collection, job->collections()) {
                    auto child = collection;
                    while (child.parentCollection() != root && child.parentCollection().isValid())
                        child = child.parentCollection();
                    if (!children.contains(child.id()))
                        children[child.id()] = child;
                }

                foreach (const auto &topLevel, children.values())
                    add(topLevel);
            });
        });

        query->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
        });
        query->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });
        query->setPredicateFunction([this, root] (const Akonadi::Collection &collection) {
            return collection.isValid() && collection.parentCollection() == root;
        });
        query->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }

    return m_findSearchChildren.value(root.id())->result();
}

void DataSourceQueries::onCollectionAdded(const Collection &collection)
{
    m_dataSourceCache->add(collection);
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onAdded(collection);
}

void DataSourceQueries::onCollectionRemoved(const Collection &collection)
{
    m_dataSourceCache->remove(collection);
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onRemoved(collection);
}

void DataSourceQueries::onCollectionChanged(const Collection &collection)
{
    m_dataSourceCache->update(collection);
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onChanged(collection);
}

DataSourceQueries::DataSourceQuery::Ptr DataSourceQueries::createDataSourceQuery()
{
    auto query = DataSourceQuery::Ptr::create();
    m_dataSourceQueries << query;
    return query;
}
