/* This file is part of Zanshin Todo.

   Copyright 2008-2011 Kevin Ottens <ervin@kde.org>

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

#include <qtest_kde.h>

#include <QStandardItemModel>
#include <QtGui/QItemSelectionModel>

#include "core/modelstack.h"
#include "testlib/mockdatastore.h"
#include "utils/dependencymanager.h"

class ModelStackTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase()
    {
        Utils::DependencyManager::globalInstance().add<DataStoreInterface, MockDataStore>();
    }

    void shouldEnsureModelsAreAvailable()
    {
        ModelStack stack;

        QVERIFY(stack.baseModel()!=0);
        QVERIFY(stack.collectionsModel()!=0);

        QVERIFY(stack.treeModel()!=0);
        QVERIFY(stack.treeSideBarModel()!=0);
        QVERIFY(stack.treeComboModel()!=0);

        QVERIFY(stack.contextsModel()!=0);
        QVERIFY(stack.contextsSideBarModel()!=0);
        QVERIFY(stack.contextsComboModel()!=0);
    }

    void shouldEnsureModelsAreConstant()
    {
        ModelStack stack;

        QList<QAbstractItemModel*> models;
        models << stack.baseModel()
               << stack.collectionsModel()
               << stack.treeModel()
               << stack.treeSideBarModel()
               << stack.treeSelectionModel()
               << stack.treeComboModel()
               << stack.contextsModel()
               << stack.contextsSideBarModel()
               << stack.contextsSelectionModel()
               << stack.contextsComboModel();

        for (int i=0; i<3; i++) {
            QList<QAbstractItemModel*> list = models;
            QCOMPARE(stack.baseModel(), list.takeFirst());
            QCOMPARE(stack.collectionsModel(), list.takeFirst());
            QCOMPARE(stack.treeModel(), list.takeFirst());
            QCOMPARE(stack.treeSideBarModel(), list.takeFirst());
            QCOMPARE(stack.treeSelectionModel(), list.takeFirst());
            QCOMPARE(stack.treeComboModel(), list.takeFirst());
            QCOMPARE(stack.contextsModel(), list.takeFirst());
            QCOMPARE(stack.contextsSideBarModel(), list.takeFirst());
            QCOMPARE(stack.contextsSelectionModel(), list.takeFirst());
            QCOMPARE(stack.contextsComboModel(), list.takeFirst());
        }
    }
};

QTEST_KDEMAIN(ModelStackTest, GUI)

#include "modelstacktest.moc"
