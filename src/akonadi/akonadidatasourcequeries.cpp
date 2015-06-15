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
#include "domain/mergedqueryresultprovider.h"

#include "utils/jobhandler.h"
#include "utils/compositejob.h"

#include <QPointer>
#include <QStringList>

#include <KCalCore/Todo>
#include <Akonadi/Notes/NoteUtils>

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

static bool matchesParents(const Collection &col, const std::function<bool(const Collection &)> &matcher)
{
    bool matches = false;
    traverseThisAndParents(col, [matcher, &matches](const Collection &c) {
        if (matcher(c)) {
            matches = true;
            return false;
        }
        return true;
    });
    return matches;
}

AkonadiCollectionTreeSource::AkonadiCollectionTreeSource(MonitorInterface *monitor)
    : QObject(),
    m_monitor(monitor),
    m_populated(false),
    m_populationInProgress(false)
{
    isToplevel = [](const Akonadi::Collection &col) {
        if (col == Akonadi::Collection::root()) {
            return true;
        }
        if (col.name() == "Other Users") {
            return true;
        }
        return false;
    };
}

Akonadi::Collection::Id AkonadiCollectionTreeSource::id(const Akonadi::Collection &col) const
{
    if (isToplevel(col)) {
        return 0;
    }
    return col.id();
}

void AkonadiCollectionTreeSource::findChildren(const Collection &parent)
{
    populate([this, parent] {
        for (auto collection : m_collections[id(parent)]) {
            emit added(collection, id(parent));
        }
    });
}

void AkonadiCollectionTreeSource::setFilter(const std::function<bool(const Akonadi::Collection &)> &filter)
{
    isWantedCollection = filter;
}

void AkonadiCollectionTreeSource::setCollectionFetcher(const std::function<void(const std::function<void(bool, const Akonadi::Collection::List&)> &)> &fetcher)
{
    fetchCollections = fetcher;
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

        fetchCollections([this, handle](bool error, const Akonadi::Collection::List &collections){
            //Since the this pointer might be invalid meanwhile, we have to double check
            if (!handle) {
                return;
            }
            if (error) {
                m_pendingCallbacks.clear();
                return;
            }
            for (auto collection : collections) {
                if (!isWantedCollection(collection)) {
                    continue;
                }
                traverseThisAndParents(collection, [this](const Collection &col) {
                    Collection::List &collections = m_collections[id(col.parentCollection())];
                    if (!collections.contains(col)) {
                        collections.append(col);
                        if (isToplevel(col)) {
                            return false;
                        } else {
                            return true;
                        }
                    }
                    return false;
                });
            }
            m_populated = true;
            if (m_monitor) {
                connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onAdded(Akonadi::Collection)));
                connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onRemoved(Akonadi::Collection)));
                connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onChanged(Akonadi::Collection)));
            }
            for (auto callback : m_pendingCallbacks) {
                callback();
            }
            m_pendingCallbacks.clear();
        });

    }
}

void AkonadiCollectionTreeSource::onAdded(const Collection &col)
{
    if (!isWantedCollection(col)) {
        return;
    }

    //Insert collection and missing parents
    traverseThisAndParents(col, [this](const Collection &col) {

        Collection::List &collections = m_collections[id(col.parentCollection())];
        if (!collections.contains(col)) {
            collections.append(col);
            emit added(col, id(col.parentCollection()));
            if (isToplevel(col)) {
                return false;
            } else {
                return true;
            }
        }
        return false;
    });
}

void AkonadiCollectionTreeSource::onRemoved(const Collection &col)
{
    //Collection may have to remain if it has wanted children.
    if (m_collections.contains(col.id())) {
        m_collections[id(col.parentCollection())].removeAll(col);
        m_collections.remove(col.id());
        emit removed(col, id(col.parentCollection()));

        //Check for parents to remove
        traverseThisAndParents(col.parentCollection(), [this](const Collection &col) {
            Collection::List &collections = m_collections[id(col)];
            if (!isWantedCollection(col) && collections.isEmpty()) {
                m_collections[id(col.parentCollection())].removeAll(col);
                m_collections.remove(col.id());
                emit removed(col, id(col.parentCollection()));
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
        Collection::List &collections = m_collections[id(col.parentCollection())];
        if (collections.contains(col)) {
            Collection::List &collections = m_collections[id(col.parentCollection())];
            collections.removeAll(col);
            collections.append(col);
            emit changed(col, id(col.parentCollection()));
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
    connect(m_source.data(), SIGNAL(added(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onAdded(Akonadi::Collection, Akonadi::Collection::Id)));
    connect(m_source.data(), SIGNAL(removed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onRemoved(Akonadi::Collection, Akonadi::Collection::Id)));
    connect(m_source.data(), SIGNAL(changed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onChanged(Akonadi::Collection, Akonadi::Collection::Id)));
}

void TreeQuery::findChildren(const Akonadi::Collection &col)
{
    if (m_source) {
        m_source->findChildren(col);
    }
}

void TreeQuery::reset(const QSharedPointer<AkonadiCollectionTreeSource> &source)
{
    disconnect(m_source.data(), SIGNAL(added(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onAdded(Akonadi::Collection, Akonadi::Collection::Id)));
    disconnect(m_source.data(), SIGNAL(removed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onRemoved(Akonadi::Collection, Akonadi::Collection::Id)));
    disconnect(m_source.data(), SIGNAL(changed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onChanged(Akonadi::Collection, Akonadi::Collection::Id)));

    m_source = source;
    foreach(auto query, m_findChildren.values()) {
        query->reset();
    }

    connect(m_source.data(), SIGNAL(added(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onAdded(Akonadi::Collection, Akonadi::Collection::Id)));
    connect(m_source.data(), SIGNAL(removed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onRemoved(Akonadi::Collection, Akonadi::Collection::Id)));
    connect(m_source.data(), SIGNAL(changed(Akonadi::Collection, Akonadi::Collection::Id)), this, SLOT(onChanged(Akonadi::Collection, Akonadi::Collection::Id)));
}

DataSourceQueries::DataSourceResult::Ptr TreeQuery::findChildren(Domain::DataSource::Ptr source, const std::function<void(DataSourceQuery::Ptr, const Akonadi::Collection &root)> &setupFunction)
{
    const Collection root = source ? m_serializer->createCollectionFromDataSource(source) : Collection::root();
    if (!m_findChildren.contains(root.id())) {
        auto query = DataSourceQuery::Ptr::create();
        m_findChildren.insert(root.id(), query);
        setupFunction(query, root);
    }
    return m_findChildren.value(root.id())->result();
}

void TreeQuery::onAdded(const Akonadi::Collection &collection, Akonadi::Collection::Id id)
{
    auto query = m_findChildren.find(id);
    if (query != m_findChildren.end()) {
        (*query)->onAdded(collection);
    }
}

void TreeQuery::onRemoved(const Akonadi::Collection &collection, Akonadi::Collection::Id id)
{
    auto query = m_findChildren.find(id);
    if (query != m_findChildren.end()) {
        (*query)->onRemoved(collection);
    }
}

void TreeQuery::onChanged(const Akonadi::Collection &collection, Akonadi::Collection::Id id)
{
    auto query = m_findChildren.find(id);
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
      m_fetchContentTypeFilter(StorageInterface::Tasks | StorageInterface::Notes)
{
    init();
}

DataSourceQueries::DataSourceQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_monitor(monitor),
      m_ownInterfaces(false),
      m_fetchContentTypeFilter(StorageInterface::Tasks | StorageInterface::Notes)
{
    init();
}

DataSourceQueries::~DataSourceQueries()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
        delete m_monitor;
    }
}

void DataSourceQueries::setApplicationMode(DataSourceQueries::ApplicationMode mode)
{
    if (mode == TasksOnly) {
        m_fetchContentTypeFilter = StorageInterface::Tasks;
    } else if (mode == NotesOnly) {
        m_fetchContentTypeFilter = StorageInterface::Notes;
    }
}

void DataSourceQueries::init()
{
    connect(m_monitor, SIGNAL(collectionAdded(Akonadi::Collection)), this, SLOT(onCollectionAdded(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(onCollectionRemoved(Akonadi::Collection)));
    connect(m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(onCollectionChanged(Akonadi::Collection)));
}

QSharedPointer<TreeQuery> DataSourceQueries::getVisibleCollectionTree() const
{
    if (!m_treeQuery) {
        DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
        self->m_treeQuery = QSharedPointer<TreeQuery>(new TreeQuery(m_serializer, findVisibleCollections()));
    }
    return m_treeQuery;
}

QSharedPointer<TreeQuery> DataSourceQueries::getVisiblePersonTree() const
{

    if (!m_personTreeQuery) {
        DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
        self->m_personTreeQuery = QSharedPointer<TreeQuery>(new TreeQuery(m_serializer, findVisiblePersonCollections()));
    }
    return m_personTreeQuery;
}

QSharedPointer<TreeQuery> DataSourceQueries::getSearchCollectionTree() const
{

    if (!m_searchTreeQuery) {
        DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
        self->m_searchTreeQuery = QSharedPointer<TreeQuery>(new TreeQuery(m_serializer, findSearchCollections()));
    }
    return m_searchTreeQuery;
}

QSharedPointer<TreeQuery> DataSourceQueries::getSearchPersonCollectionTree() const
{

    if (!m_searchPersonTreeQuery) {
        DataSourceQueries *self = const_cast<DataSourceQueries*>(this);
        self->m_searchPersonTreeQuery = QSharedPointer<TreeQuery>(new TreeQuery(m_serializer, findSearchPersonCollections()));
    }
    return m_searchPersonTreeQuery;
}

QSharedPointer<AkonadiCollectionTreeSource> DataSourceQueries::findVisibleCollections() const
{
    auto source = QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_monitor));
    source->setFilter([this](const Collection &col) {
        if (!col.shouldList(Collection::ListDisplay) && !col.referenced()) {
            return false;
        }
        if(matchesParents(col, [this](const Collection &col) {
            if (col.name() == "Other Users") {
                return true;
            }
            //Filter the search collection
            if (col.id() == 1) {
                return true;
            }
            return false;
        })) {
            return false;
        }

        //filter by mimetype because of updates from monitor
        //TODO the mimetype checking should be implemented in either storage or serializer
        if ((m_fetchContentTypeFilter & StorageInterface::Tasks && !col.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) ||
            (m_fetchContentTypeFilter & StorageInterface::Notes && !col.contentMimeTypes().contains(NoteUtils::noteMimeType()))) {
            return false;
        }
        return true;
    });
    source->setCollectionFetcher([this](const std::function<void(bool, const Akonadi::Collection::List&)> &resultHandler) {
        auto job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive, m_fetchContentTypeFilter, StorageInterface::NoFilter);
        Utils::JobHandler::install(job->kjob(), [job, resultHandler] {
            if (job->kjob()->error()) {
                kWarning() << "Failed to fetch collections " << job->kjob()->errorString();
                resultHandler(true, Akonadi::Collection::List());
                return;
            }
            resultHandler(false, job->collections());
        });
    });

    return source;
}

QSharedPointer<AkonadiCollectionTreeSource> DataSourceQueries::findVisiblePersonCollections() const
{
    auto source = QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_monitor));
    source->setFilter([this](const Collection &col) {
            if (!col.shouldList(Collection::ListDisplay) && !col.referenced()) {
                return false;
            }
            if(!matchesParents(col, [this](const Collection &col) {
                return m_serializer->isPersonCollection(col);
            })) {
                return false;
            }

            //filter by mimetype because of updates from monitor
            //TODO the mimetype checking should be implemented in either storage or serializer
            if ((m_fetchContentTypeFilter & StorageInterface::Tasks && !col.contentMimeTypes().contains(KCalCore::Todo::todoMimeType())) ||
                (m_fetchContentTypeFilter & StorageInterface::Notes && !col.contentMimeTypes().contains(NoteUtils::noteMimeType()))) {
                return false;
            }
            return true;
    });
    source->setCollectionFetcher([this](const std::function<void(bool, const Akonadi::Collection::List&)> &resultHandler) {
        auto job = m_storage->fetchPersons();
        Utils::JobHandler::install(job->kjob(), [this, job, resultHandler] {
            if (job->kjob()->error()) {
                kWarning() << "Failed to search persons " << job->kjob()->errorString();
                resultHandler(true, Akonadi::Collection::List());
                return;
            }
            auto result = QSharedPointer<Akonadi::Collection::List>::create();
            result->append(job->collections());
            //Fetch children for each person
            auto compositeJob = new Utils::CompositeJob;
            for (const auto &col : job->collections()) {
                auto fetchJob = m_storage->fetchCollections(col, StorageInterface::Recursive, m_fetchContentTypeFilter, StorageInterface::NoFilter);
                compositeJob->install(fetchJob->kjob(), [fetchJob, resultHandler, result] {
                    result->append(fetchJob->collections());
                });
            }
            Utils::JobHandler::install(compositeJob, [result, resultHandler] {
                resultHandler(false, *result);
            });
        });
    });

    return source;
}

QSharedPointer<AkonadiCollectionTreeSource> DataSourceQueries::findSearchCollections() const
{
    auto source = QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_monitor));
    source->setFilter([this](const Collection &col) {
        if(matchesParents(col, [this](const Collection &col) {
            if (m_serializer->isPersonCollection(col)) {
                return true;
            }
            //Filter the search collection
            if (col.id() == 1) {
                return true;
            }
            return false;
        })) {
            return false;
        }
        return true;
    });
    source->setCollectionFetcher([this](const std::function<void(bool, const Akonadi::Collection::List&)> &resultHandler) {
        auto job = m_storage->searchCollections(m_searchTerm, m_fetchContentTypeFilter);
        Utils::JobHandler::install(job->kjob(), [job, resultHandler] {
            if (job->kjob()->error()) {
                kWarning() << "Failed to search collections " << job->kjob()->errorString();
                resultHandler(true, Akonadi::Collection::List());
                return;
            }
            resultHandler(false, job->collections());
        });
    });

    return source;
}

QSharedPointer<AkonadiCollectionTreeSource> DataSourceQueries::findSearchPersonCollections() const
{
    auto source = QSharedPointer<AkonadiCollectionTreeSource>(new AkonadiCollectionTreeSource(m_monitor));
    source->setFilter([this](const Collection &col) {
        if(!matchesParents(col, [this](const Collection &col) {
            return m_serializer->isPersonCollection(col);
        })) {
            return false;
        }
        return true;
    });
    source->setCollectionFetcher([this](const std::function<void(bool, const Akonadi::Collection::List&)> &resultHandler) {
        //Search for persons, and then search for all their children
        auto job = m_storage->searchPersons(m_searchTerm);
        Utils::JobHandler::install(job->kjob(), [this, job, resultHandler] {
            if (job->kjob()->error()) {
                kWarning() << "Failed to search persons " << job->kjob()->errorString();
                resultHandler(true, Akonadi::Collection::List());
                return;
            }
            auto result = QSharedPointer<Akonadi::Collection::List>::create();
            result->append(job->collections());
            //Fetch children for each person
            auto compositeJob = new Utils::CompositeJob;
            for (const auto &col : job->collections()) {
                auto fetchJob = m_storage->fetchCollections(col, StorageInterface::Recursive, m_fetchContentTypeFilter, StorageInterface::NoFilter);
                compositeJob->install(fetchJob->kjob(), [fetchJob, resultHandler, result] {
                    result->append(fetchJob->collections());
                });
            }
            Utils::JobHandler::install(compositeJob, [result, resultHandler] {
                resultHandler(false, *result);
            });
        });
    });

    return source;
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

    Domain::MergedQueryResultProvider<Domain::DataSource::Ptr>::Ptr mergedResultProvider(new Domain::MergedQueryResultProvider<Domain::DataSource::Ptr>());
    mergedResultProvider->addQueryResult(findSearchChildrenQuery(Domain::DataSource::Ptr(), getVisibleCollectionTree(), false));
    mergedResultProvider->addQueryResult(findSearchChildrenQuery(Domain::DataSource::Ptr(), getVisiblePersonTree(), true));
    return DataSourceQueries::DataSourceResult::create(mergedResultProvider);
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findChildren(Domain::DataSource::Ptr source) const
{
    QSharedPointer<TreeQuery> treeQuery;
    //The person tree and the rest are separate, so we need to multiplex that here
    const bool isPersonTree = source->property("isInPersonTree").toBool();
    if (source && isPersonTree) {
        treeQuery = getVisiblePersonTree();
    } else {
        treeQuery = getVisibleCollectionTree();
    }
    return findSearchChildrenQuery(source, treeQuery, isPersonTree);
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findChildrenRecursive(Domain::DataSource::Ptr source) const
{
    auto query = Domain::QueryResultProvider<Domain::DataSource::Ptr>::Ptr::create();
    Akonadi::Collection col = m_serializer->createCollectionFromDataSource(source);

    //We have to ignore the content type, because the mimetypes are not yet available
    auto job = m_storage->fetchCollections(col, StorageInterface::Recursive, 0, StorageInterface::NoFilter);
    Utils::JobHandler::install(job->kjob(), [this, job, query] {
        for (auto collection : job->collections()) {
            auto source =  m_serializer->createDataSourceFromCollection(collection, SerializerInterface::FullPath);
            query->append(source);
        }
        query->done();
    });
    return DataSourceQueries::DataSourceResult::create(query);
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

    getSearchCollectionTree()->reset(findSearchCollections());
    getSearchPersonCollectionTree()->reset(findSearchPersonCollections());
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findSearchChildrenQuery(Domain::DataSource::Ptr source, const QSharedPointer<TreeQuery> &treeQuery, bool isPersonTree) const
{
    return treeQuery->findChildren(source, [this, treeQuery, isPersonTree](DataSourceQuery::Ptr query, const Akonadi::Collection &root){
        query->setFetchFunction([treeQuery, root] (const DataSourceQuery::AddFunction &add) {
            //We don't need the add function because we use the usual delivery method via the onAdded slot used for updates
            Q_UNUSED(add);
            treeQuery->findChildren(root);
        });

        query->setConvertFunction([this, isPersonTree] (const Akonadi::Collection &collection) {
            auto source = m_serializer->createDataSourceFromCollection(collection, SerializerInterface::BaseName);
            source->setProperty("isInPersonTree", isPersonTree);
            return source;
        });

        query->setUpdateFunction([this] (const Akonadi::Collection &collection, Domain::DataSource::Ptr &source) {
            m_serializer->updateDataSourceFromCollection(source, collection, SerializerInterface::BaseName);
        });

        query->setPredicateFunction([this, root] (const Akonadi::Collection &) {
            return true;
        });

        query->setRepresentsFunction([this] (const Akonadi::Collection &collection, const Domain::DataSource::Ptr &source) {
            return m_serializer->representsCollection(source, collection);
        });
    });
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findSearchTopLevel() const
{
    Domain::MergedQueryResultProvider<Domain::DataSource::Ptr>::Ptr mergedResultProvider(new Domain::MergedQueryResultProvider<Domain::DataSource::Ptr>());
    //FIXME pass in function to fetch children for persons and rest? => see below for motivation in findSearchChildren
    mergedResultProvider->addQueryResult(findSearchChildrenQuery(Domain::DataSource::Ptr(), getSearchCollectionTree(), false));
    mergedResultProvider->addQueryResult(findSearchChildrenQuery(Domain::DataSource::Ptr(), getSearchPersonCollectionTree(), true));
    return DataSourceQueries::DataSourceResult::create(mergedResultProvider);
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findSearchChildren(Domain::DataSource::Ptr source) const
{
    //FIXME findSearchChildren should be separate for regular collections and persons
    QSharedPointer<TreeQuery> treeQuery;
    //The person tree and the rest are separate, so we need to multiplex that here
    const bool isPersonTree = source->property("isInPersonTree").toBool();
    if (source && isPersonTree) {
        treeQuery = getSearchPersonCollectionTree();
    } else {
        treeQuery = getSearchCollectionTree();
    }
    return findSearchChildrenQuery(source, treeQuery, isPersonTree);
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
