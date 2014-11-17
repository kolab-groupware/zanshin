/* This file is part of Zanshin

   Copyright 2014 Mario Bensi <mbensi@ipsquad.net>
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

#include <QColor>

#include <mockitopp/mockitopp.hpp>

#include "domain/taskqueries.h"
#include "domain/taskrepository.h"
#include "presentation/querytreemodel.h"
#include "testlib/modeltest.h"

using namespace mockitopp;

Q_DECLARE_METATYPE(QModelIndex)
Q_DECLARE_METATYPE(QList<QColor>)

class QueryTreeModelTest : public QObject
{
    Q_OBJECT
public:
    explicit QueryTreeModelTest(QObject *parent = 0)
        : QObject(parent)
    {
        qRegisterMetaType<QModelIndex>();
    }

private:
    Domain::Task::List createTasks() const
    {
        Domain::Task::List result;

        const QStringList titles = {"first", "second", "third"};
        const QList<bool> doneStates = {true, false, false};
        Q_ASSERT(titles.size() == doneStates.size());

        for (int i = 0; i < titles.size(); i++) {
            auto task = Domain::Task::Ptr::create();
            task->setTitle(titles.at(i));
            task->setDone(doneStates.at(i));
            result << task;
        }

        return result;
    }

    Domain::Task::List createChildrenTasks() const
    {
        Domain::Task::List result;

        const QStringList titles = {"childFirst", "childSecond", "childThird"};
        const QList<bool> doneStates = {true, false, false};
        Q_ASSERT(titles.size() == doneStates.size());

        for (int i = 0; i < titles.size(); i++) {
            auto task = Domain::Task::Ptr::create();
            task->setTitle(titles.at(i));
            task->setDone(doneStates.at(i));
            result << task;
        }

        return result;
    }

private slots:
    void shouldHaveRoleNames()
    {
        // GIVEN
        auto queryGenerator = [](const QColor &) {
            return Domain::QueryResult<QColor>::Ptr();
        };
        auto flagsFunction = [](const QColor &) {
            return Qt::NoItemFlags;
        };
        auto dataFunction = [](const QColor &, int) {
            return QVariant();
        };
        auto setDataFunction = [](const QColor &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<QColor> model(queryGenerator, flagsFunction, dataFunction, setDataFunction);

        // WHEN
        auto roles = model.roleNames();

        // THEN
        QCOMPARE(roles.value(Qt::DisplayRole), QByteArray("display"));
        QCOMPARE(roles.value(Presentation::QueryTreeModel<QColor>::ObjectRole), QByteArray("object"));
        QCOMPARE(roles.value(Presentation::QueryTreeModel<QColor>::IconNameRole), QByteArray("icon"));
    }

    void shouldListTasks()
    {
        // GIVEN
        auto tasks = createTasks();
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);

        // WHEN
        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);

        // THEN
        QCOMPARE(model.rowCount(), 3);
        QCOMPARE(model.rowCount(model.index(0, 0)), 3);
        QCOMPARE(model.rowCount(model.index(1, 0)), 0);
        QCOMPARE(model.rowCount(model.index(2, 0)), 0);
        QCOMPARE(model.rowCount(model.index(0, 0, model.index(0, 0))), 0);
        QCOMPARE(model.rowCount(model.index(1, 0, model.index(0, 0))), 0);
        QCOMPARE(model.rowCount(model.index(2, 0, model.index(0, 0))), 0);
        QCOMPARE(model.rowCount(model.index(3, 0, model.index(0, 0))), 3);

        for (int i = 0; i < tasks.size(); i++) {
            auto task = tasks.at(i);
            auto index = model.index(i, 0);

            QCOMPARE(model.data(index), model.data(index, Qt::DisplayRole));
            QCOMPARE(model.data(index).toString(), task->title());
            QCOMPARE(model.data(index, Qt::CheckStateRole).toInt() == Qt::Checked, task->isDone());
            QCOMPARE(model.data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked, !task->isDone());
        }

        for (int i = 0; i < childrenTasks.size(); i++) {
            auto task = childrenTasks.at(i);
            auto index = model.index(i, 0, model.index(0, 0));

            QCOMPARE(model.data(index), model.data(index, Qt::DisplayRole));
            QCOMPARE(model.data(index).toString(), task->title());
            QCOMPARE(model.data(index, Qt::CheckStateRole).toInt() == Qt::Checked, task->isDone());
            QCOMPARE(model.data(index, Qt::CheckStateRole).toInt() == Qt::Unchecked, !task->isDone());
        }
    }

    void shouldDealWithNullQueriesProperly()
    {
        // GIVEN
        auto queryGenerator = [](const QString &) {
            return Domain::QueryResult<QString>::Ptr();
        };
        auto flagsFunction = [](const QString &) {
            return Qt::NoItemFlags;
        };
        auto dataFunction = [](const QString &, int) {
            return QVariant();
        };
        auto setDataFunction = [](const QString &, const QVariant &, int) {
            return false;
        };

        // WHEN
        Presentation::QueryTreeModel<QString> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);

        // THEN
        QCOMPARE(model.rowCount(), 0);
    }

    void shouldReactToTaskAdd()
    {
        // GIVEN
        auto tasks = createTasks();
        auto  provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        provider->append(tasks.at(1));
        provider->append(tasks.at(2));

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);

        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);
        QSignalSpy aboutToBeInsertedSpy(&model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
        QSignalSpy insertedSpy(&model, SIGNAL(rowsInserted(QModelIndex, int, int)));

        // WHEN
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);
        provider->insert(0, tasks.at(0));

        // THEN
        QCOMPARE(aboutToBeInsertedSpy.size(), 1);
        QCOMPARE(aboutToBeInsertedSpy.first().at(0).value<QModelIndex>(), QModelIndex());
        QCOMPARE(aboutToBeInsertedSpy.first().at(1).toInt(), 0);
        QCOMPARE(aboutToBeInsertedSpy.first().at(2).toInt(), 0);
        QCOMPARE(insertedSpy.size(), 1);
        QCOMPARE(insertedSpy.first().at(0).value<QModelIndex>(), QModelIndex());
        QCOMPARE(insertedSpy.first().at(1).toInt(), 0);
        QCOMPARE(insertedSpy.first().at(2).toInt(), 0);
    }

    void shouldReactToChilrenTaskAdd()
    {
        // GIVEN
        auto tasks = createTasks();
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        childrenProvider->append(childrenTasks.at(0));
        childrenProvider->append(childrenTasks.at(1));

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);

        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);
        QSignalSpy aboutToBeInsertedSpy(&model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
        QSignalSpy insertedSpy(&model, SIGNAL(rowsInserted(QModelIndex, int, int)));

        // WHEN
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);
        childrenProvider->insert(1, tasks.at(2));

        // THEN
        QCOMPARE(aboutToBeInsertedSpy.size(), 1);
        QCOMPARE(aboutToBeInsertedSpy.first().at(0).value<QModelIndex>(), model.index(0, 0));
        QCOMPARE(aboutToBeInsertedSpy.first().at(1).toInt(), 1);
        QCOMPARE(aboutToBeInsertedSpy.first().at(2).toInt(), 1);
        QCOMPARE(insertedSpy.size(), 1);
        QCOMPARE(insertedSpy.first().at(0).value<QModelIndex>(), model.index(0, 0));
        QCOMPARE(insertedSpy.first().at(1).toInt(), 1);
        QCOMPARE(insertedSpy.first().at(2).toInt(), 1);
    }

    void shouldReactToTaskRemove()
    {
        // GIVEN
        auto tasks = createTasks();
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);

        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);
        QSignalSpy aboutToBeRemovedSpy(&model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));
        QSignalSpy removedSpy(&model, SIGNAL(rowsRemoved(QModelIndex, int, int)));
        QSignalSpy aboutToBeInsertedSpy(&model, SIGNAL(rowsAboutToBeInserted(QModelIndex, int, int)));
        QSignalSpy insertedSpy(&model, SIGNAL(rowsInserted(QModelIndex, int, int)));

        QModelIndex removeIndex = model.index(0, 0);

        // WHEN
        // Remove children
        childrenProvider->removeAt(0);
        childrenProvider->removeAt(0);
        childrenProvider->removeAt(0);
        // Move children to Top Level
        provider->append(childrenTasks.at(0));
        provider->append(childrenTasks.at(1));
        provider->append(childrenTasks.at(2));
        // Remove firt element from topLevel
        provider->removeAt(0);

        // THEN
        QCOMPARE(aboutToBeRemovedSpy.size(), 4);
        QCOMPARE(removedSpy.size(), 4);
        for (int i = 0; i < aboutToBeRemovedSpy.size(); i++) {
            if (i != 3)
                QCOMPARE(aboutToBeRemovedSpy.at(i).at(0).value<QModelIndex>(), removeIndex);
            else
                QCOMPARE(aboutToBeRemovedSpy.at(i).at(0).value<QModelIndex>(), QModelIndex());
            QCOMPARE(aboutToBeRemovedSpy.at(i).at(1).toInt(), 0);
            QCOMPARE(aboutToBeRemovedSpy.at(i).at(2).toInt(), 0);

            if (i != 3)
                QCOMPARE(removedSpy.at(i).at(0).value<QModelIndex>(), removeIndex);
            else
                QCOMPARE(removedSpy.at(i).at(0).value<QModelIndex>(), QModelIndex());
            QCOMPARE(removedSpy.at(i).at(1).toInt(), 0);
            QCOMPARE(removedSpy.at(i).at(2).toInt(), 0);
        }

        QCOMPARE(aboutToBeInsertedSpy.size(), 3);
        QCOMPARE(insertedSpy.size(), 3);
        for (int i = 0; i < aboutToBeInsertedSpy.size(); i++) {
            QCOMPARE(aboutToBeInsertedSpy.at(i).at(0).value<QModelIndex>(), QModelIndex());
            QCOMPARE(aboutToBeInsertedSpy.at(i).at(1).toInt(), i + 3);
            QCOMPARE(aboutToBeInsertedSpy.at(i).at(2).toInt(), i + 3);
            QCOMPARE(insertedSpy.at(i).at(0).value<QModelIndex>(), QModelIndex());
            QCOMPARE(insertedSpy.at(i).at(1).toInt(), i + 3);
            QCOMPARE(insertedSpy.at(i).at(1).toInt(), i + 3);
        }
    }

    void shouldReactToTaskChange()
    {
        // GIVEN
        // GIVEN
        auto tasks = createTasks();
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);

        // WHEN
        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);
        QSignalSpy dataChangedSpy(&model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));

        // WHEN
        tasks.at(2)->setDone(true);
        childrenTasks.at(2)->setDone(true);
        provider->replace(2, tasks.at(2));
        childrenProvider->replace(2, tasks.at(2));

        // THEN
        QCOMPARE(dataChangedSpy.size(), 2);
        QCOMPARE(dataChangedSpy.first().at(0).value<QModelIndex>(), model.index(2, 0));
        QCOMPARE(dataChangedSpy.first().at(1).value<QModelIndex>(), model.index(2, 0));
        QCOMPARE(dataChangedSpy.last().at(0).value<QModelIndex>(), model.index(2, 0, model.index(0, 0)));
        QCOMPARE(dataChangedSpy.last().at(1).value<QModelIndex>(), model.index(2, 0, model.index(0, 0)));
    }

    void shouldAllowEditsAndChecks()
    {
        // GIVEN
        auto tasks = createTasks();
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);

        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);

        mock_object<Domain::TaskQueries> queryMock;
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);

        // WHEN
        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [](const Domain::Task::Ptr &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);

        // WHEN
        // Nothing particular

        // THEN
        for (int row = 0; row < tasks.size(); row++) {
            QVERIFY(model.flags(model.index(row, 0)) & Qt::ItemIsEditable);
            QVERIFY(model.flags(model.index(row, 0)) & Qt::ItemIsUserCheckable);
        }
        for (int row = 0; row < childrenTasks.size(); row++) {
            QVERIFY(model.flags(model.index(row, 0, model.index(0, 0))) & Qt::ItemIsEditable);
            QVERIFY(model.flags(model.index(row, 0, model.index(0, 0))) & Qt::ItemIsUserCheckable);
        }
    }

    void shouldSaveChanges()
    {
        // GIVEN
        auto tasks = createTasks();
        const int taskPos = 1;
        const auto task = tasks[taskPos];
        auto provider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : tasks)
            provider->append(task);

        auto childrenTasks = createChildrenTasks();
        auto childrenProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        for (auto task : childrenTasks)
            childrenProvider->append(task);
        
        auto childrenList = Domain::QueryResult<Domain::Task::Ptr>::create(childrenProvider);
        auto emptyProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto emptyList = Domain::QueryResult<Domain::Task::Ptr>::create(emptyProvider);
            
        mock_object<Domain::TaskQueries> queryMock; 
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(0)).thenReturn(childrenList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(tasks.at(2)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(0)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(1)).thenReturn(emptyList);
        queryMock(&Domain::TaskQueries::findChildren).when(childrenTasks.at(2)).thenReturn(emptyList);

        mock_object<Domain::TaskRepository> repositoryMock;
        repositoryMock(&Domain::TaskRepository::update).when(task).thenReturn(0);

        auto queryGenerator = [&](const Domain::Task::Ptr &task) {
            if (!task)
                return Domain::QueryResult<Domain::Task::Ptr>::create(provider);
            else
                return queryMock.getInstance().findChildren(task);
        };
        auto flagsFunction = [](const Domain::Task::Ptr &) {
            return Qt::ItemIsSelectable
                 | Qt::ItemIsEnabled
                 | Qt::ItemIsEditable
                 | Qt::ItemIsUserCheckable;
        };
        auto dataFunction = [](const Domain::Task::Ptr &task, int role) -> QVariant {
            if (role != Qt::DisplayRole && role != Qt::CheckStateRole) {
                return QVariant();
            }

            if (role == Qt::DisplayRole)
                return task->title();
            else
                return task->isDone() ? Qt::Checked : Qt::Unchecked;
        };
        auto setDataFunction = [&](const Domain::Task::Ptr &task, const QVariant &value, int role) {
            if (role != Qt::EditRole && role != Qt::CheckStateRole) {
                return false;
            }

            if (role == Qt::EditRole) {
                task->setTitle(value.toString());
            } else {
                task->setDone(value.toInt() == Qt::Checked);
            }

            repositoryMock.getInstance().update(task);
            return true;
        };
        Presentation::QueryTreeModel<Domain::Task::Ptr> model(queryGenerator, flagsFunction, dataFunction, setDataFunction, 0);
        new ModelTest(&model);
        QSignalSpy titleChangedSpy(task.data(), SIGNAL(titleChanged(QString)));
        QSignalSpy doneChangedSpy(task.data(), SIGNAL(doneChanged(bool)));

        // WHEN
        const auto index = model.index(taskPos, 0);
        model.setData(index, "alternate second");
        model.setData(index, Qt::Checked, Qt::CheckStateRole);

        // THEN
        QVERIFY(repositoryMock(&Domain::TaskRepository::update).when(task).exactly(2));

        QCOMPARE(titleChangedSpy.size(), 1);
        QCOMPARE(titleChangedSpy.first().first().toString(), QString("alternate second"));
        QCOMPARE(doneChangedSpy.size(), 1);
        QCOMPARE(doneChangedSpy.first().first().toBool(), true);
    }

    void shouldProvideUnderlyingObject()
    {
        // GIVEN
        auto provider = Domain::QueryResultProvider<QColor>::Ptr::create();
        provider->append(Qt::red);
        provider->append(Qt::green);
        provider->append(Qt::blue);

        auto queryGenerator = [&](const QColor &color) {
            if (!color.isValid())
                return Domain::QueryResult<QColor>::create(provider);
            else
                return Domain::QueryResult<QColor>::Ptr();
        };
        auto flagsFunction = [](const QColor &) {
            return Qt::NoItemFlags;
        };
        auto dataFunction = [](const QColor &, int) {
            return QVariant();
        };
        auto setDataFunction = [](const QColor &, const QVariant &, int) {
            return false;
        };
        Presentation::QueryTreeModel<QColor> model(queryGenerator, flagsFunction, dataFunction, setDataFunction);
        new ModelTest(&model);

        // WHEN
        const QModelIndex index = model.index(1, 0);
        const QVariant data = index.data(Presentation::QueryTreeModel<QColor>::ObjectRole);

        // THEN
        QVERIFY(data.isValid());
        QCOMPARE(data.value<QColor>(), provider->data().at(1));
    }

    void shouldCreateMimeData()
    {
        // GIVEN
        auto provider = Domain::QueryResultProvider<QColor>::Ptr::create();
        provider->append(Qt::red);
        provider->append(Qt::green);
        provider->append(Qt::blue);

        auto queryGenerator = [&] (const QColor &color) {
            if (!color.isValid())
                return Domain::QueryResult<QColor>::create(provider);
            else
                return Domain::QueryResult<QColor>::Ptr();
        };
        auto flagsFunction = [] (const QColor &) {
            return Qt::NoItemFlags;
        };
        auto dataFunction = [] (const QColor &, int) {
            return QVariant();
        };
        auto setDataFunction = [] (const QColor &, const QVariant &, int) {
            return false;
        };
        auto dropFunction = [] (const QMimeData *, Qt::DropAction, const QColor &) {
            return false;
        };
        auto dragFunction = [] (const QList<QColor> &colors) {
            auto mimeData = new QMimeData;
            mimeData->setColorData(QVariant::fromValue(colors));
            return mimeData;
        };

        Presentation::QueryTreeModel<QColor> model(queryGenerator, flagsFunction,
                                                   dataFunction, setDataFunction,
                                                   dropFunction, dragFunction);
        new ModelTest(&model);

        // WHEN
        auto data = model.mimeData(QList<QModelIndex>() << model.index(1, 0) << model.index(2, 0));

        // THEN
        QVERIFY(data);
        QVERIFY(model.mimeTypes().contains("application/x-zanshin-object"));
        QList<QColor> colors;
        colors << Qt::green << Qt::blue;
        QCOMPARE(data->colorData().value<QList<QColor>>(), colors);
    }

    void shouldDropMimeData_data()
    {
        QTest::addColumn<int>("row");
        QTest::addColumn<int>("column");
        QTest::addColumn<int>("parentRow");
        QTest::addColumn<bool>("callExpected");

        QTest::newRow("drop on object") << -1 << -1 << 2 << true;
        QTest::newRow("drop between object") << 1 << 0 << -1 << false;
        QTest::newRow("drop in empty area") << -1 << -1 << -1 << true;
    }

    void shouldDropMimeData()
    {
        // GIVEN
        QFETCH(int, row);
        QFETCH(int, column);
        QFETCH(int, parentRow);
        QFETCH(bool, callExpected);
        bool dropCalled = false;
        const QMimeData *droppedData = 0;
        QColor colorSeen;

        auto provider = Domain::QueryResultProvider<QColor>::Ptr::create();
        provider->append(Qt::red);
        provider->append(Qt::green);
        provider->append(Qt::blue);

        auto queryGenerator = [&] (const QColor &color) {
            if (!color.isValid())
                return Domain::QueryResult<QColor>::create(provider);
            else
                return Domain::QueryResult<QColor>::Ptr();
        };
        auto flagsFunction = [] (const QColor &) {
            return Qt::NoItemFlags;
        };
        auto dataFunction = [] (const QColor &, int) {
            return QVariant();
        };
        auto setDataFunction = [] (const QColor &, const QVariant &, int) {
            return false;
        };
        auto dropFunction = [&] (const QMimeData *data, Qt::DropAction, const QColor &color) {
            dropCalled = true;
            droppedData = data;
            colorSeen = color;
            return false;
        };
        auto dragFunction = [] (const QList<QColor> &) -> QMimeData* {
            return 0;
        };

        Presentation::QueryTreeModel<QColor> model(queryGenerator, flagsFunction,
                                                   dataFunction, setDataFunction,
                                                   dropFunction, dragFunction);
        new ModelTest(&model);

        // WHEN
        auto data = new QMimeData;
        const QModelIndex parent = parentRow >= 0 ? model.index(parentRow, 0) : QModelIndex();
        model.dropMimeData(data, Qt::MoveAction, row, column, parent);

        // THEN
        QCOMPARE(dropCalled, callExpected);
        if (callExpected) {
            QCOMPARE(droppedData, data);
            QCOMPARE(colorSeen, parent.data(Presentation::QueryTreeModelBase::ObjectRole).value<QColor>());
        }
    }
};

QTEST_MAIN(QueryTreeModelTest)

#include "querytreemodeltest.moc"
