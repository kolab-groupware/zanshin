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


#include "akonadidatasourcerepository.h"

#include "akonadiserializer.h"
#include "akonadistorage.h"
#include <QMenu>
#include <Akonadi/Calendar/StandardCalendarActionManager>
#include <Akonadi/EntityTreeModel>
#include <KActionCollection>
#include <KAction>
#include <KLocalizedString>
#include <QWidget>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <KCalCore/Todo>
#include <akonadi/notes/noteutils.h>
#include <akonadi/collectionpropertiesdialog.h>
#include <akonadi/attributefactory.h>
#include <pimcommon/acl/collectionaclpage.h>
#include <pimcommon/acl/imapaclattribute.h>


using namespace Akonadi;

DataSourceRepository::DataSourceRepository(QObject *parent)
    : QObject(parent),
      m_storage(new Storage),
      m_serializer(new Serializer),
      m_ownInterfaces(true)
{
}

DataSourceRepository::DataSourceRepository(StorageInterface *storage, SerializerInterface *serializer)
    : m_storage(storage),
      m_serializer(serializer),
      m_ownInterfaces(false)
{
}

DataSourceRepository::~DataSourceRepository()
{
    if (m_ownInterfaces) {
        delete m_storage;
        delete m_serializer;
    }
}

KJob *DataSourceRepository::update(Domain::DataSource::Ptr source)
{
    auto collection = m_serializer->createCollectionFromDataSource(source);
    Q_ASSERT(collection.isValid());
    return m_storage->updateCollection(collection);
}

void DataSourceRepository::configure(QMenu *menu , Domain::DataSource::Ptr selectedSource)
{
    {
        static bool pageRegistered = false;
        if (!pageRegistered) {
            kDebug() << "registering pages";
            // Akonadi::CollectionPropertiesDialog::registerPage(new CalendarSupport::CollectionGeneralPageFactory);
            Akonadi::CollectionPropertiesDialog::registerPage(new PimCommon::CollectionAclPageFactory);
            pageRegistered = true;
        }
    }

    if (menu->isEmpty()) {
        //We bind the lifetime of all actions to the menu and simply recreate the menu everytime.
        QObject *parent = menu;
        //This can could be used to provide a parent widget for the created dialogs
        QWidget *parentWidget = 0;

        static auto actionCollection = new KActionCollection(0, KComponentData());
        //We can't delete the actionmanager because it's the parent of running jobs
        static Akonadi::StandardCalendarActionManager *mActionManager = 0;
        if (!mActionManager) {
            mActionManager = new Akonadi::StandardCalendarActionManager(actionCollection, parentWidget);

            QList<Akonadi::StandardActionManager::Type> standardActions;
            standardActions << Akonadi::StandardActionManager::CreateCollection
                            << Akonadi::StandardActionManager::DeleteCollections
                            << Akonadi::StandardActionManager::SynchronizeCollections
                            << Akonadi::StandardActionManager::CollectionProperties;

            Q_FOREACH(Akonadi::StandardActionManager::Type standardAction, standardActions) {
                mActionManager->createAction(standardAction);
            }

            const QStringList pages = QStringList() << QLatin1String("CalendarSupport::CollectionGeneralPage")
                                                    << QLatin1String("Akonadi::CachePolicyPage")
                                                    << QLatin1String("PimCommon::CollectionAclPage");
            mActionManager->setCollectionPropertiesPageNames(pages);
        }
        
        const auto collection = m_serializer->createCollectionFromDataSource(selectedSource);

        //Since we have no ETM based selection model we simply emulate one to tell the actionmanager about the current collection
        auto itemModel = new QStandardItemModel(parent);
        auto selectionModel = new QItemSelectionModel(itemModel);

        auto item = new QStandardItem();
        item->setData(QVariant::fromValue(collection), Akonadi::EntityTreeModel::CollectionRole);
        itemModel->setItem(0, 0, item);

        mActionManager->setCollectionSelectionModel(selectionModel);

        selectionModel->setCurrentIndex(itemModel->index(0, 0, QModelIndex()), QItemSelectionModel::SelectCurrent);

        //FIXME set appropriate mimetypes based on selected source
        if (m_serializer->isTaskCollection(collection)) {
            mActionManager->setActionText(Akonadi::StandardActionManager::CreateCollection, ki18n("Add Task Folder"));
            mActionManager->setActionText(Akonadi::StandardActionManager::DeleteCollections, ki18n("Delete Task Folder"));
            mActionManager->setActionText(Akonadi::StandardActionManager::SynchronizeCollections, ki18n("Update Folder"));
            mActionManager->setContextText(Akonadi::StandardActionManager::CollectionProperties,
                                            Akonadi::StandardActionManager::DialogTitle,
                                            tr("@title:window", "Properties of Task Folder %1"));
            mActionManager->action(Akonadi::StandardActionManager::CreateCollection )->setProperty("ContentMimeTypes",
                                        QStringList() << Akonadi::Collection::mimeType() << KCalCore::Todo::todoMimeType());
        } else if (m_serializer->isNoteCollection(collection)) {
            mActionManager->setActionText(Akonadi::StandardActionManager::CreateCollection, ki18n("Add Note Folder"));
            mActionManager->setActionText(Akonadi::StandardActionManager::DeleteCollections, ki18n("Delete Note Folder"));
            mActionManager->setActionText(Akonadi::StandardActionManager::SynchronizeCollections, ki18n("Update Folder"));
            mActionManager->setContextText(Akonadi::StandardActionManager::CollectionProperties,
                                            Akonadi::StandardActionManager::DialogTitle,
                                            tr("@title:window", "Properties of Note Folder %1"));
            mActionManager->action(Akonadi::StandardActionManager::CreateCollection )->setProperty("ContentMimeTypes",
                                        QStringList() << Akonadi::Collection::mimeType() << Akonadi::NoteUtils::noteMimeType());
        }

        menu->addActions(actionCollection->actions());
    } else {
        //update actions according to selected datasource
        //TODO currently we always recreate all actions and menues 
    }
}
