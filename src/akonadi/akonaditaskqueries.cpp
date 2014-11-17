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

#include "utils/jobhandler.h"

using namespace Akonadi;

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
    Akonadi::Item item = m_serializer->createItemFromTask(task);   
    if (!m_findChildren.contains(item.id())) {
        TaskQuery::Ptr query;

        {
            TaskQueries *self = const_cast<TaskQueries*>(this);
            query = self->createTaskQuery();
            self->m_findChildren.insert(item.id(), query);
        }

        query->setFetchFunction([this, item] (const TaskQuery::AddFunction &add) {
            ItemFetchJobInterface *job = m_storage->fetchItem(item);
            Utils::JobHandler::install(job->kjob(), [this, job, add] {
                if (job->kjob()->error() != KJob::NoError)
                    return;

                Q_ASSERT(job->items().size() == 1);
                auto item = job->items()[0];
                Q_ASSERT(item.parentCollection().isValid());
                ItemFetchJobInterface *job = m_storage->fetchItems(item.parentCollection());
                Utils::JobHandler::install(job->kjob(), [this, job, add] {
                    if (job->kjob()->error() != KJob::NoError)
                        return;

                    for (auto item : job->items())
                        add(item);
                });
            });
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
    }

    return m_findChildren.value(item.id())->result();
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
