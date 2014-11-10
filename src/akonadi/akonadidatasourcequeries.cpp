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

#include <QPointer>

using namespace Akonadi;

static void traverseThisAndParents(const Collection &col, const std::function<bool(const Collection &)> &f)
{
    auto topLevel = col;
    while (topLevel.isValid() && topLevel != Collection::root()) {
        if (!f(topLevel)) {
            return;
        }
        topLevel = topLevel.parentCollection();
    }
}

AkonadiCollectionTreeSource::AkonadiCollectionTreeSource(StorageInterface *storage, MonitorInterface *monitor)
    : QObject(),
    m_storage(storage),
    m_monitor(monitor),
    m_populated(false),
    m_populationInProgress(false)
{
}

void AkonadiCollectionTreeSource::findChildren(const Collection &parent)
{
    populate([this, parent] {
        for (auto collection : m_collections[parent.id()]) {
            emit added(collection);
        }
    });
}

void AkonadiCollectionTreeSource::populate(const std::function<void()> &callback)
{
    if (m_populated) {
        callback();
    } else if (m_populationInProgress) {
        m_pendingCallbacks << callback;
    } else {
        m_populationInProgress = true;
        m_pendingCallbacks << callback;
        QPointer<AkonadiCollectionTreeSource> handle(this);
        CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive, StorageInterface::Tasks | StorageInterface::Notes);
        Utils::JobHandler::install(job->kjob(), [this, job, handle] {
            //Since the this pointer might be invalid meanwhile, we have to double check
            if (!handle) {
                return;
            }
            if (job->kjob()->error()) {
                kWarning() << "Failed to fetch collections " << job->kjob()->errorString();
                m_pendingCallbacks.clear();
                return;
            }
            for (auto collection : job->collections()) {
                traverseThisAndParents(collection, [this](const Collection &col) {
                    Collection::List &collections = m_collections[col.parentCollection().id()];
                    if (!collections.contains(col)) {
                        collections.append(col);
                    }
                    return true;
                });
            }
            m_populated = true;
            connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onAdded(Akonadi::Collection)));
            connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onRemoved(Akonadi::Collection)));
            connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onChanged(Akonadi::Collection)));
            for (auto callback : m_pendingCallbacks) {
                callback();
            }
            m_pendingCallbacks.clear();
        });
    }
}

static bool isWantedCollection(const Collection &col)
{
    if (!col.shouldList(Collection::ListDisplay) && !col.referenced()) {
        return false;
    }
    return true;
}

void AkonadiCollectionTreeSource::onAdded(const Collection &col)
{
    if (!isWantedCollection(col)) {
        return;
    }

    //Insert collection and missing parents
    traverseThisAndParents(col, [this](const Collection &col) {
        Collection::List &collections = m_collections[col.parentCollection().id()];
        if (!collections.contains(col)) {
            collections.append(col);
            emit added(col);
            return true;
        }
        return false;
    });
}

void AkonadiCollectionTreeSource::onRemoved(const Collection &col)
{
    //Collection may have to remain if it has wanted children.
    if (m_collections.contains(col.id())) {
        m_collections[col.parentCollection().id()].removeAll(col);
        m_collections.remove(col.id());
        emit removed(col);

        //Check for parents to remove
        traverseThisAndParents(col.parentCollection(), [this](const Collection &col) {
            Collection::List &collections = m_collections[col.id()];
            if (!isWantedCollection(col) && collections.isEmpty()) {
                m_collections[col.parentCollection().id()].removeAll(col);
                m_collections.remove(col.id());
                emit removed(col);
            }
            return true;
        });
    }
}

void AkonadiCollectionTreeSource::onChanged(const Collection &col)
{
    if (!isWantedCollection(col)) {
        onRemoved(col);
    } else {
        Collection::List &collections = m_collections[col.parentCollection().id()];
        if (collections.contains(col)) {
            Collection::List &collections = m_collections[col.parentCollection().id()];
            collections.removeAll(col);
            collections.append(col);
            emit changed(col);
        } else {
            onAdded(col);
        }
    }
}


TreeQuery::TreeQuery(SerializerInterface *serializer, const QSharedPointer<AkonadiCollectionTreeSource> &source)
    : QObject(),
    m_serializer(serializer),
    m_source(source)
{
    connect(m_source.data(), SIGNAL(added(Akonadi::Collection)), this, SLOT(onAdded(Akonadi::Collection)));
    connect(m_source.data(), SIGNAL(removed(Akonadi::Collection)), this, SLOT(onRemoved(Akonadi::Collection)));
    connect(m_source.data(), SIGNAL(changed(Akonadi::Collection)), this, SLOT(onChanged(Akonadi::Collection)));
}

DataSourceQueries::DataSourceResult::Ptr TreeQuery::findChildren(Domain::DataSource::Ptr source)
{
    Collection root = source ? m_serializer->createCollectionFromDataSource(source) : Collection::root();
    if (!m_findChildren.contains(root.id())) {
        auto query = DataSourceQuery::Ptr::create();
        m_findChildren.insert(root.id(), query);

        query->setFetchFunction([this, root] (const DataSourceQuery::AddFunction &add) {
            //We don't need the add function because we use the usual delivery method via the onAdded slot used for updates
            Q_UNUSED(add);
            m_source->findChildren(root);
        });

        query->setConvertFunction([this] (const Akonadi::Collection &collection) {
            return m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
        });

        query->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });

        query->setPredicateFunction([this, root] (const Akonadi::Collection &collection) {
            return true;
        });

        query->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    }
    return m_findChildren.value(root.id())->result();
}

void TreeQuery::onAdded(const Akonadi::Collection &collection)
{
    auto query = m_findChildren.find(collection.parentCollection().id());
    if (query != m_findChildren.end()) {
        (*query)->onAdded(collection);
    }
}

void TreeQuery::onRemoved(const Akonadi::Collection &collection)
{
    auto query = m_findChildren.find(collection.parentCollection().id());
    if (query != m_findChildren.end()) {
        (*query)->onRemoved(collection);
    }
}

void TreeQuery::onChanged(const Akonadi::Collection &collection)
{
    auto query = m_findChildren.find(collection.parentCollection().id());
    if (query != m_findChildren.end()) {
        (*query)->onChanged(collection);
    }
}


DataSourceQueries::DataSourceQueries(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_monitor(new MonitorImpl),
      m_ownInterfaces(true),
      m_treeQuery(new TreeQuery(m_serializer, QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_storage, m_monitor))))
{
    connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onCollectionAdded(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onCollectionRemoved(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onCollectionChanged(Akonadi::Collection)));
}

DataSourceQueries::DataSourceQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_monitor(monitor),
      m_ownInterfaces(false),
      m_treeQuery(new TreeQuery(m_serializer, QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_storage, m_monitor))))
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
    return m_treeQuery->findChildren(Domain::DataSource::Ptr());
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findChildren(Domain::DataSource::Ptr source) const
{
    return m_treeQuery->findChildren(source);
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
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onAdded(collection);
}

void DataSourceQueries::onCollectionRemoved(const Collection &collection)
{
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onRemoved(collection);
}

void DataSourceQueries::onCollectionChanged(const Collection &collection)
{
    foreach (const DataSourceQuery::Ptr &query, m_dataSourceQueries)
        query->onChanged(collection);
}

DataSourceQueries::DataSourceQuery::Ptr DataSourceQueries::createDataSourceQuery()
{
    auto query = DataSourceQuery::Ptr::create();
    m_dataSourceQueries << query;
    return query;
}
