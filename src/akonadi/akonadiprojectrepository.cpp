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


#include "akonadiprojectrepository.h"

#include "akonadiitemfetchjobinterface.h"
#include "akonadiserializer.h"
#include "akonadistorage.h"

#include "utils/compositejob.h"

using namespace Akonadi;

ProjectRepository::ProjectRepository(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_ownInterfaces(true)
{
}

ProjectRepository::ProjectRepository(StorageInterface *storage, SerializerInterface *serializer)
    : m_storage(storage),
      m_serializer(serializer),
      m_ownInterfaces(false)
{
}

ProjectRepository::~ProjectRepository()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
    }
}

KJob *ProjectRepository::create(Domain::Project::Ptr project, Domain::DataSource::Ptr source)
{
    auto item = m_serializer->createItemFromProject(project);
    Q_ASSERT(!item.isValid());
    auto collection = m_serializer->createCollectionFromDataSource(source);
    Q_ASSERT(collection.isValid());
    return m_storage->createItem(item, collection);
}

KJob *ProjectRepository::update(Domain::Project::Ptr project)
{
    auto item = m_serializer->createItemFromProject(project);
    Q_ASSERT(item.isValid());
    return m_storage->updateItem(item);
}

KJob *ProjectRepository::remove(Domain::Project::Ptr project)
{
    auto item = m_serializer->createItemFromProject(project);
    Q_ASSERT(item.isValid());
    return m_storage->removeItem(item);
}

KJob *ProjectRepository::associate(Domain::Project::Ptr parent, Domain::Artifact::Ptr child)
{
    Item childItem;
    if (auto task = child.objectCast<Domain::Task>())
        childItem = m_serializer->createItemFromTask(task);
    else if (auto note = child.objectCast<Domain::Note>())
        childItem = m_serializer->createItemFromNote(note);
    Q_ASSERT(childItem.isValid());

    auto job = new Utils::CompositeJob();
    ItemFetchJobInterface *fetchItemJob = m_storage->fetchItem(childItem);
    job->install(fetchItemJob->kjob(), [fetchItemJob, parent, child, job, this] {
        if (fetchItemJob->kjob()->error() != KJob::NoError)
           return;

        Q_ASSERT(fetchItemJob->items().size() == 1);
        auto childItem = fetchItemJob->items().first();
        m_serializer->updateItemProject(childItem, parent);

        // Check collections to know if we need to move child
        auto parentItem = m_serializer->createItemFromProject(parent);
        ItemFetchJobInterface *fetchParentItemJob = m_storage->fetchItem(parentItem);
        job->install(fetchParentItemJob->kjob(), [fetchParentItemJob, child, childItem, job, this] {
            if (fetchParentItemJob->kjob()->error() != KJob::NoError)
                return;

            Q_ASSERT(fetchParentItemJob->items().size() == 1);
            auto parentItem = fetchParentItemJob->items().first();

            const int itemCollectionId = childItem.parentCollection().id();
            const int parentCollectionId = parentItem.parentCollection().id();

            if (child.objectCast<Domain::Task>()
             && itemCollectionId != parentCollectionId) {
                ItemFetchJobInterface *fetchChildrenItemJob = m_storage->fetchItems(childItem.parentCollection());
                job->install(fetchChildrenItemJob->kjob(), [fetchChildrenItemJob, childItem, parentItem, job, this] {
                    if (fetchChildrenItemJob->kjob()->error() != KJob::NoError)
                        return;

                    Item::List childItems = m_serializer->filterDescendantItems(fetchChildrenItemJob->items(), childItem);

                    auto transaction = m_storage->createTransaction();
                    m_storage->updateItem(childItem, transaction);
                    childItems.push_front(childItem);
                    m_storage->moveItems(childItems, parentItem.parentCollection(), transaction);
                    job->addSubjob(transaction);
                    transaction->start();
                });
            } else {
                auto updateJob = m_storage->updateItem(childItem);
                job->addSubjob(updateJob);
                updateJob->start();
            }
        });
    });

    return job;
}

KJob *ProjectRepository::dissociate(Domain::Artifact::Ptr child)
{
    auto job = new Utils::CompositeJob();
    const auto task = child.objectCast<Domain::Task>();
    const auto note = child.objectCast<Domain::Note>();

    const auto childItem = task ? m_serializer->createItemFromTask(task)
                         : note ? m_serializer->createItemFromNote(note)
                         : Akonadi::Item();
    Q_ASSERT(childItem.isValid());

    ItemFetchJobInterface *fetchItemJob = m_storage->fetchItem(childItem);
    job->install(fetchItemJob->kjob(), [fetchItemJob, job, this] {
        if (fetchItemJob->kjob()->error() != KJob::NoError)
            return;

        Q_ASSERT(fetchItemJob->items().size() == 1);
        auto childItem = fetchItemJob->items().first();

        m_serializer->removeItemParent(childItem);

        auto updateJob = m_storage->updateItem(childItem);
        job->addSubjob(updateJob);
        updateJob->start();
    });

    return job;
}
