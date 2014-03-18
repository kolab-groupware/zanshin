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
#include "akonadimonitorinterface.h"
#include "akonadiserializerinterface.h"
#include "akonadistorageinterface.h"

using namespace Akonadi;

TaskQueries::TaskQueries()
    : m_storage(0),
      m_serializer(0),
      m_monitor(0)
{
}

TaskQueries::TaskQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor)
    : m_storage(storage),
      m_serializer(serializer),
      m_monitor(monitor)
{
    connect(monitor, SIGNAL(itemAdded(Item)), this, SLOT(onItemAdded(Item)));
    connect(monitor, SIGNAL(itemRemoved(Item)), this, SLOT(onItemRemoved(Item)));
}

TaskQueries::~TaskQueries()
{
}

TaskQueries::TaskResult::Ptr TaskQueries::findAll() const
{
    TaskProvider::Ptr provider(m_taskProvider.toStrongRef());

    if (!provider) {
        provider = TaskProvider::Ptr(new TaskProvider);
        m_taskProvider = provider.toWeakRef();
    }

    TaskQueries::TaskResult::Ptr result = TaskProvider::createResult(provider);

    CollectionFetchJobInterface *job = m_storage->fetchCollections(Akonadi::Collection::root(), StorageInterface::Recursive);
    registerJobHandler(job->kjob(), [provider, job, this] {
        for (auto collection : job->collections()) {
            ItemFetchJobInterface *job = m_storage->fetchItems(collection);
            registerJobHandler(job->kjob(), [provider, job, this] {
                for (auto item : job->items()) {
                    provider->append(deserializeTask(item));
                }
            });
        }
    });

    return result;
}

TaskQueries::ArtifactResult::Ptr TaskQueries::findChildren(const Domain::Task::Ptr &task) const
{
    qFatal("Not implemented yet");
    Q_UNUSED(task);
    return ArtifactProvider::createResult(ArtifactProvider::Ptr());
}

void TaskQueries::onItemAdded(const Item &item)
{
    TaskProvider::Ptr provider(m_taskProvider.toStrongRef());

    if (provider) {
        provider->append(deserializeTask(item));
    }
}

void TaskQueries::onItemRemoved(const Item &item)
{
    TaskProvider::Ptr provider(m_taskProvider.toStrongRef());

    if (provider) {
        for (int i = 0; i < provider->data().size(); i++) {
            auto task = provider->data().at(i);
            if (isTaskItem(task, item)) {
                provider->removeAt(i);
                i--;
            }
        }
    }
}

void TaskQueries::handleJobResult(KJob *job)
{
    Q_ASSERT(m_jobHandlers.contains(job));
    auto handler = m_jobHandlers.take(job);
    handler();
}

void TaskQueries::registerJobHandler(KJob *job, const std::function<void()> &handler) const
{
    connect(job, SIGNAL(result(KJob*)), this, SLOT(handleJobResult(KJob*)));
    m_jobHandlers.insert(job, handler);
}

bool TaskQueries::isTaskItem(const Domain::Task::Ptr &task, const Item &item) const
{
    return task->property("itemId").toLongLong() == item.id();
}

Domain::Task::Ptr TaskQueries::deserializeTask(const Item &item) const
{
    auto task = m_serializer->deserializeTask(item);
    task->setProperty("itemId", item.id());
    return task;
}