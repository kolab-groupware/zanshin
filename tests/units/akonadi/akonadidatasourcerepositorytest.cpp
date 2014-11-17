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

#include <QtTest>

#include <mockitopp/mockitopp.hpp>
#include "testlib/akonadimocks.h"

#include "akonadi/akonadidatasourcerepository.h"
#include "akonadi/akonadiserializerinterface.h"
#include "akonadi/akonadistorageinterface.h"

using namespace mockitopp;

Q_DECLARE_METATYPE(MockItemFetchJob*)

class AkonadiDataSourceRepositoryTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldUpdateExistingSources()
    {
        // GIVEN

        // A source and its corresponding collection already existing in storage
        Akonadi::Collection collection(42);
        auto source = Domain::DataSource::Ptr::create();

        // A mock modify job
        auto collectionModifyJob = new MockAkonadiJob(this);

        // Storage mock returning the create job
        mock_object<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::updateCollection).when(collection, 0)
                                                                 .thenReturn(collectionModifyJob);

        // Serializer mock returning the item for the project
        mock_object<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createCollectionFromDataSource).when(source).thenReturn(collection);

        // WHEN
        QScopedPointer<Akonadi::DataSourceRepository> repository(new Akonadi::DataSourceRepository(&storageMock.getInstance(),
                                                                                                   &serializerMock.getInstance()));
        repository->update(source)->exec();

        // THEN
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createCollectionFromDataSource).when(source).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::updateCollection).when(collection, 0).exactly(1));
    }
};

QTEST_MAIN(AkonadiDataSourceRepositoryTest)

#include "akonadidatasourcerepositorytest.moc"
