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


#include "akonaditaskqueries.h"

#include "akonadicollectionfetchjobinterface.h"
#include "akonadiitemfetchjobinterface.h"
#include "akonadimonitorimpl.h"
#include "akonadiserializer.h"
#include "akonadistorage.h"

#include <QPointer>
#include <KCalCore/Todo>

#include "utils/jobhandler.h"

using namespace Akonadi;

AkonadiItemSource::AkonadiItemSource(MonitorInterface *monitor)
    : QObject(),
    m_monitor(monitor),
    m_populated(false),
    m_populationInProgress(false)
{
    isToplevel = [](const Akonadi::Collection &col) {
        if (col == Akonadi::Collection::root()) {
            return true;
        }
        // if (col.name() == "Other Users") {
        //     return true;
        // }
        return false;
    };
}

Akonadi::Collection::Id AkonadiItemSource::id(const Akonadi::Collection &col) const
{
    if (isToplevel(col)) {
        return 0;
    }
    return col.id();
}

void AkonadiItemSource::findChildren(const Item &item)
{
    QString parent;
    try {
        auto todo = item.payload<KCalCore::Todo::Ptr>();
        parent = todo->uid();
    } catch (...) {

    }
    populate([this, parent] {
        for (auto item : m_items[parent]) {
            emit added(item, parent);
        }
    });
}

void AkonadiItemSource::setFilter(const std::function<bool(const Akonadi::Item &)> &filter)
{
    isWantedItem = filter;
}

void AkonadiItemSource::setItemFetcher(const std::function<void(const std::function<void(bool, const Akonadi::Item::List&)> &)> &fetcher)
{
    fetchItems = fetcher;
}

void AkonadiItemSource::populate(const std::function<void()> &callback)
{
    if (m_populated) {
        callback();
    } else if (m_populationInProgress) {
        m_pendingCallbacks << callback;
    } else {
        m_populationInProgress = true;
        m_pendingCallbacks << callback;
        QPointer<AkonadiItemSource> handle(this);

        fetchItems([this, handle](bool error, const Akonadi::Item::List &items){
            //Since the this pointer might be invalid meanwhile, we have to double check
            if (!handle) {
                return;
            }
            if (error) {
                m_pendingCallbacks.clear();
                return;
            }
            for (auto item : items) {
                try {
                    auto todo = item.payload<KCalCore::Todo::Ptr>();
                    m_items[todo->relatedTo()].append(item);
                } catch (...) {

                }
            }
            m_populated = true;
            if (m_monitor) {
                connect(m_monitor, SIGNAL(itemAdded(Akonadi::Item)), this, SLOT(onAdded(Akonadi::Item)));
                connect(m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SLOT(onRemoved(Akonadi::Item)));
                connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item)), this, SLOT(onChanged(Akonadi::Item)));
                connect(m_monitor, SIGNAL(itemMoved(Akonadi::Item)), this, SLOT(onChanged(Akonadi::Item)));
            }
            for (auto callback : m_pendingCallbacks) {
                callback();
            }
            m_pendingCallbacks.clear();
        });

    }
}

void AkonadiItemSource::onAdded(const Item &item)
{
    if (!isWantedItem(item)) {
        return;
    }
    QString parent;
    try {
        auto todo = item.payload<KCalCore::Todo::Ptr>();
        parent = todo->relatedTo();
    } catch (...) {

    }
    emit added(item, parent);
}

void AkonadiItemSource::onRemoved(const Item &item)
{
    QString parent;
    emit removed(item, parent);
}

void AkonadiItemSource::onChanged(const Item &item)
{
    QString parent;
    try {
        auto todo = item.payload<KCalCore::Todo::Ptr>();
        parent = todo->relatedTo();
    } catch (...) {

    }
    emit changed(item, parent);
}


TaskTreeQuery::TaskTreeQuery(SerializerInterface *serializer, const QSharedPointer<AkonadiItemSource> &source)
    : QObject(),
    m_serializer(serializer),
    m_source(source)
{
    connect(m_source.data(), SIGNAL(added(Akonadi::Item, QString)), this, SLOT(onAdded(Akonadi::Item, QString)));
    connect(m_source.data(), SIGNAL(removed(Akonadi::Item, QString)), this, SLOT(onRemoved(Akonadi::Item, QString)));
    connect(m_source.data(), SIGNAL(changed(Akonadi::Item, QString)), this, SLOT(onChanged(Akonadi::Item, QString)));
}

void TaskTreeQuery::findChildren(const Akonadi::Item &item)
{
    if (m_source) {
        m_source->findChildren(item);
    }
}

void TaskTreeQuery::reset(const QSharedPointer<AkonadiItemSource> &source)
{
    disconnect(m_source.data(), SIGNAL(added(Akonadi::Item, QString)), this, SLOT(onAdded(Akonadi::Item, QString)));
    disconnect(m_source.data(), SIGNAL(removed(Akonadi::Item, QString)), this, SLOT(onRemoved(Akonadi::Item, QString)));
    disconnect(m_source.data(), SIGNAL(changed(Akonadi::Item, QString)), this, SLOT(onChanged(Akonadi::Item, QString)));

    m_source = source;
    foreach(auto query, m_findChildren.values()) {
        query->reset();
    }

    connect(m_source.data(), SIGNAL(added(Akonadi::Item, QString)), this, SLOT(onAdded(Akonadi::Item, QString)));
    connect(m_source.data(), SIGNAL(removed(Akonadi::Item, QString)), this, SLOT(onRemoved(Akonadi::Item, QString)));
    connect(m_source.data(), SIGNAL(changed(Akonadi::Item, QString)), this, SLOT(onChanged(Akonadi::Item, QString)));
}

TaskTreeQuery::Result::Ptr TaskTreeQuery::findChildren(Domain::Task::Ptr parent, const std::function<void(Query::Ptr, const Akonadi::Collection &root)> &setupFunction)
{
    const auto parentUid = parent->property("todoUid").toString();
    const auto item = m_serializer->createItemFromTask(parent);
    if (!m_findChildren.contains(parentUid)) {
        auto query = Query::Ptr::create();
        m_findChildren.insert(parentUid, query);
        setupFunction(query, item.parentCollection());
    }
    return m_findChildren.value(parentUid)->result();
}

void TaskTreeQuery::onAdded(const Akonadi::Item &item, const QString &parent)
{
    auto query = m_findChildren.find(parent);
    if (query != m_findChildren.end()) {
        (*query)->onAdded(item);
    }
}

void TaskTreeQuery::onRemoved(const Akonadi::Item &item, const QString &parent)
{
    for (auto query : m_findChildren.values()) {
        query->onRemoved(item);
    }
}

void TaskTreeQuery::onChanged(const Akonadi::Item &item, const QString &parent)
{
    for (auto query : m_findChildren.values()) {
        query->onChanged(item);
    }
}

TaskQueries::TaskQueries(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_monitor(new MonitorImpl),
      m_ownInterfaces(true)
{
    connect(m_monitor, SIGNAL(itemAdded(Akonadi::Item)), this, SLOT(onItemAdded(Akonadi::Item)));
    connect(m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SLOT(onItemRemoved(Akonadi::Item)));
    connect(m_monitor, SIGNAL(itemChanged(Akonadi::Item)), this, SLOT(onItemChanged(Akonadi::Item)));
}

TaskQueries::TaskQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_monitor(monitor),
      m_ownInterfaces(false)
{
    connect(monitor, SIGNAL(itemAdded(Akonadi::Item)), this, SLOT(onItemAdded(Akonadi::Item)));
    connect(monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SLOT(onItemRemoved(Akonadi::Item)));
    connect(monitor, SIGNAL(itemChanged(Akonadi::Item)), this, SLOT(onItemChanged(Akonadi::Item)));
}

TaskQueries::~TaskQueries()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
        delete m_monitor;
    }
}

QSharedPointer<TaskTreeQuery> TaskQueries::getTaskTree(const Akonadi::Collection &parent) const
{
    if (!m_treeQueries.contains(parent.id())) {
        TaskQueries *self = const_cast<TaskQueries*>(this);
        self->m_treeQueries.insert(parent.id(), QSharedPointer<TaskTreeQuery>(new TaskTreeQuery(m_serializer, findTaskTree(parent))));
    }
    return m_treeQueries.value(parent.id());
}

QSharedPointer<AkonadiItemSource> TaskQueries::findTaskTree(const Akonadi::Collection &parentCollection) const
{
    auto source = QSharedPointer<AkonadiItemSource>(new AkonadiItemSource(m_monitor));
    source->setFilter([this, parentCollection](const Item &item) {
        return (parentCollection.id() == item.parentCollection().id());
    });
    source->setItemFetcher([this, parentCollection](const std::function<void(bool, const Akonadi::Item::List&)> &resultHandler) {
        auto job = m_storage->fetchItems(parentCollection);
        Utils::JobHandler::install(job->kjob(), [job, resultHandler] {
            if (job->kjob()->error()) {
                kWarning() << "Failed to fetch collections " << job->kjob()->errorString();
                resultHandler(true, Akonadi::Item::List());
                return;
            }
            resultHandler(false, job->items());
        });
    });

    return source;
}

TaskQueries::TaskResult::Ptr TaskQueries::findAll() const
{
    if (!m_findAll) {
        {
            TaskQueries *self = const_cast<TaskQueries*>(this);
            self->m_findAll = self->createTaskQuery();
        }

        m_findAll->setFetchFunction([this] (const TaskQuery::AddFunction &add) {
            CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(),
                                                                           StorageInterface::Recursive,
                                                                           StorageInterface::Tasks);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                if (job->kjob()->error() != KJob::NoError)
                    return;

                for (auto collection : job->collections()) {
                    ItemFetchJobInterface *job = m_storage->fetchItems(collection);
                    Utils::JobHandler::install(job->kjob(), [this, job, add] {
                        if (job->kjob()->error() != KJob::NoError)
                            return;

                        for (auto item : job->items()) {
                            add(item);
                        }
                    });
                }
            });
        });

        m_findAll->setConvertFunction([this] (const Akonadi::Item &item) {
            return m_serializer->createTaskFromItem(item);
        });
        m_findAll->setUpdateFunction([this] (const Akonadi::Item &item, Domain::Task::Ptr &task) {
            m_serializer->updateTaskFromItem(task, item);
        });
        m_findAll->setPredicateFunction([this] (const Akonadi::Item &item) {
            return m_serializer->isTaskItem(item);
        });
        m_findAll->setRepresentsFunction([this] (const Akonadi::Item &item, const Domain::Task::Ptr &task) {
            return m_serializer->representsItem(task, item);
        });
    }

    return m_findAll->result();
}

TaskQueries::TaskResult::Ptr TaskQueries::findChildren(Domain::Task::Ptr task) const
{
    auto item = m_serializer->createItemFromTask(task);
    Q_ASSERT(item.parentCollection().isValid());
    auto treeQuery = getTaskTree(item.parentCollection());
    return treeQuery->findChildren(task, [this, item, task, treeQuery](TaskQuery::Ptr query, const Akonadi::Collection &root) {
        query->setFetchFunction([this, item, treeQuery] (const TaskQuery::AddFunction &add) {
            //We don't need the add function because we use the usual delivery method via the onAdded slot used for updates
            Q_UNUSED(add);
            treeQuery->findChildren(item);
        });

        query->setConvertFunction([this] (const Akonadi::Item &item) {
            return m_serializer->createTaskFromItem(item);
        });
        query->setUpdateFunction([this] (const Akonadi::Item &item, Domain::Task::Ptr &task) {
            m_serializer->updateTaskFromItem(task, item);
        });
        query->setPredicateFunction([this, task] (const Akonadi::Item &item) {
            return m_serializer->isTaskChild(task, item);
        });
        query->setRepresentsFunction([this] (const Akonadi::Item &item, const Domain::Task::Ptr &task) {
            return m_serializer->representsItem(task, item);
        });
    });
}

TaskQueries::TaskResult::Ptr TaskQueries::findTopLevel() const
{
    if (!m_findTopLevel) {
        {
            TaskQueries *self = const_cast<TaskQueries*>(this);
            self->m_findTopLevel = self->createTaskQuery();
        }

        m_findTopLevel->setFetchFunction([this] (const TaskQuery::AddFunction &add) {
            CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(),
                                                                           StorageInterface::Recursive,
                                                                           StorageInterface::Tasks);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                if (job->kjob()->error() != KJob::NoError)
                    return;

                for (auto collection : job->collections()) {
                    ItemFetchJobInterface *job = m_storage->fetchItems(collection);
                    Utils::JobHandler::install(job->kjob(), [this, job, add] {
                        if (job->kjob()->error() != KJob::NoError)
                            return;

                        for (auto item : job->items()) {
                            add(item);
                        }
                    });
                }
            });
        });

        m_findTopLevel->setConvertFunction([this] (const Akonadi::Item &item) {
            return m_serializer->createTaskFromItem(item);
        });
        m_findTopLevel->setUpdateFunction([this] (const Akonadi::Item &item, Domain::Task::Ptr &task) {
            m_serializer->updateTaskFromItem(task, item);
        });
        m_findTopLevel->setPredicateFunction([this] (const Akonadi::Item &item) {
            return m_serializer->relatedUidFromItem(item).isEmpty();
        });
        m_findTopLevel->setRepresentsFunction([this] (const Akonadi::Item &item, const Domain::Task::Ptr &task) {
            return m_serializer->representsItem(task, item);
        });
    }

    return m_findTopLevel->result();
}

TaskQueries::ContextResult::Ptr TaskQueries::findContexts(Domain::Task::Ptr task) const
{
    qFatal("Not implemented yet");
    Q_UNUSED(task);
    return ContextResult::Ptr();
}

void TaskQueries::onItemAdded(const Item &item)
{
    foreach (const TaskQuery::Ptr &query, m_taskQueries)
        query->onAdded(item);
}

void TaskQueries::onItemRemoved(const Item &item)
{
    foreach (const TaskQuery::Ptr &query, m_taskQueries)
        query->onRemoved(item);

    if (m_findChildren.contains(item.id())) {
        auto query = m_findChildren.take(item.id());
        m_taskQueries.removeAll(query);
    }
}

void TaskQueries::onItemChanged(const Item &item)
{
    foreach (const TaskQuery::Ptr &query, m_taskQueries)
        query->onChanged(item);
}

TaskQueries::TaskQuery::Ptr TaskQueries::createTaskQuery()
{
    auto query = TaskQueries::TaskQuery::Ptr::create();
    m_taskQueries << query;
    return query;
}
