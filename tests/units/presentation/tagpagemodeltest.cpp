/* This file is part of Zanshin

   Copyright 2014 Kevin Ottens <ervin@kde.org>
   Copyright 2014 Franck Arrecot <franck.arrecot@gmail.com>

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

#include "domain/noterepository.h"
#include "domain/tagqueries.h"
#include "domain/taskqueries.h"
#include "domain/taskrepository.h"

#include "presentation/tagpagemodel.h"

#include "testlib/fakejob.h"

using namespace mockitopp;
using namespace mockitopp::matcher;

class TagPageModelTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldListTagArtifactsInCentralListModel()
    {
        // GIVEN

        // One Tag
        auto tag = Domain::Tag::Ptr::create();

        // One note and one task
        auto rootTask = Domain::Task::Ptr::create();
        rootTask->setTitle("rootTask");
        auto rootNote = Domain::Note::Ptr::create();
        rootNote->setTitle("rootNote");
        auto artifactProvider = Domain::QueryResultProvider<Domain::Artifact::Ptr>::Ptr::create();
        auto artifactResult = Domain::QueryResult<Domain::Artifact::Ptr>::create(artifactProvider);
        artifactProvider->append(rootTask);
        artifactProvider->append(rootNote);

        // One task under the root task
        auto childTask = Domain::Task::Ptr::create();
        childTask->setTitle("childTask");
        childTask->setDone(true);
        auto taskProvider = Domain::QueryResultProvider<Domain::Task::Ptr>::Ptr::create();
        auto taskResult = Domain::QueryResult<Domain::Task::Ptr>::create(taskProvider);
        taskProvider->append(childTask);

        mock_object<Domain::TagQueries> tagQueriesMock;
        tagQueriesMock(&Domain::TagQueries::findTopLevelArtifacts).when(tag).thenReturn(artifactResult);

        mock_object<Domain::TaskQueries> taskQueriesMock;
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(rootTask).thenReturn(taskResult);
        taskQueriesMock(&Domain::TaskQueries::findChildren).when(childTask).thenReturn(Domain::QueryResult<Domain::Task::Ptr>::Ptr());

        mock_object<Domain::TaskRepository> taskRepositoryMock;
        mock_object<Domain::NoteRepository> noteRepositoryMock;

        Presentation::TagPageModel page(tag,
                                        &tagQueriesMock.getInstance(),
                                        &taskQueriesMock.getInstance(),
                                        &taskRepositoryMock.getInstance(),
                                        &noteRepositoryMock.getInstance());

        // WHEN
        QAbstractItemModel *model = page.centralListModel();

        // THEN
        const QModelIndex rootTaskIndex = model->index(0, 0);
        const QModelIndex rootNoteIndex = model->index(1, 0);
        const QModelIndex childTaskIndex = model->index(0, 0, rootTaskIndex);

        QCOMPARE(page.tag(), tag);

        QCOMPARE(model->rowCount(), 2);
        QCOMPARE(model->rowCount(rootTaskIndex), 1);
        QCOMPARE(model->rowCount(rootNoteIndex), 0);
        QCOMPARE(model->rowCount(childTaskIndex), 0);

        const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable
                                         | Qt::ItemIsEnabled
                                         | Qt::ItemIsEditable
                                         | Qt::ItemIsDragEnabled;
        QCOMPARE(model->flags(rootTaskIndex), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);
        QCOMPARE(model->flags(rootNoteIndex), defaultFlags);
        QCOMPARE(model->flags(childTaskIndex), defaultFlags | Qt::ItemIsUserCheckable | Qt::ItemIsDropEnabled);

        QCOMPARE(model->data(rootTaskIndex).toString(), rootTask->title());
        QCOMPARE(model->data(rootNoteIndex).toString(), rootNote->title());
        QCOMPARE(model->data(childTaskIndex).toString(), childTask->title());

        QCOMPARE(model->data(rootTaskIndex, Qt::EditRole).toString(), rootTask->title());
        QCOMPARE(model->data(rootNoteIndex, Qt::EditRole).toString(), rootNote->title());
        QCOMPARE(model->data(childTaskIndex, Qt::EditRole).toString(), childTask->title());

        QVERIFY(model->data(rootTaskIndex, Qt::CheckStateRole).isValid());
        QVERIFY(!model->data(rootNoteIndex, Qt::CheckStateRole).isValid());
        QVERIFY(model->data(childTaskIndex, Qt::CheckStateRole).isValid());

        QCOMPARE(model->data(rootTaskIndex, Qt::CheckStateRole).toBool(), rootTask->isDone());
        QCOMPARE(model->data(childTaskIndex, Qt::CheckStateRole).toBool(), childTask->isDone());

        // WHEN
        taskRepositoryMock(&Domain::TaskRepository::update).when(rootTask).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::update).when(childTask).thenReturn(new FakeJob(this));
        noteRepositoryMock(&Domain::NoteRepository::save).when(rootNote).thenReturn(new FakeJob(this));

        QVERIFY(model->setData(rootTaskIndex, "newRootTask"));
        QVERIFY(model->setData(rootNoteIndex, "newRootNote"));
        QVERIFY(model->setData(childTaskIndex, "newChildTask"));

        QVERIFY(model->setData(rootTaskIndex, Qt::Checked, Qt::CheckStateRole));
        QVERIFY(!model->setData(rootNoteIndex, Qt::Checked, Qt::CheckStateRole));
        QVERIFY(model->setData(childTaskIndex, Qt::Unchecked, Qt::CheckStateRole));

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(rootTask).exactly(2));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::update).when(childTask).exactly(2));
        QVERIFY(noteRepositoryMock(&Domain::NoteRepository::save).when(rootNote).exactly(1));

        QCOMPARE(rootTask->title(), QString("newRootTask"));
        QCOMPARE(rootNote->title(), QString("newRootNote"));
        QCOMPARE(childTask->title(), QString("newChildTask"));

        QCOMPARE(rootTask->isDone(), true);
        QCOMPARE(childTask->isDone(), false);

        // WHEN
        QMimeData *data = model->mimeData(QModelIndexList() << childTaskIndex);

        // THEN
        QVERIFY(data->hasFormat("application/x-zanshin-object"));
        QCOMPARE(data->property("objects").value<Domain::Artifact::List>(),
                 Domain::Artifact::List() << childTask);

        // WHEN
        data = model->mimeData(QModelIndexList() << rootNoteIndex);

        // THEN
        QVERIFY(data->hasFormat("application/x-zanshin-object"));
        QCOMPARE(data->property("objects").value<Domain::Artifact::List>(),
                 Domain::Artifact::List() << rootNote);


        // WHEN
        auto childTask2 = Domain::Task::Ptr::create();
        taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask2).thenReturn(new FakeJob(this));
        data = new QMimeData;
        data->setData("application/x-zanshin-object", "object");
        data->setProperty("objects", QVariant::fromValue(Domain::Artifact::List() << childTask2));
        model->dropMimeData(data, Qt::MoveAction, -1, -1, rootTaskIndex);

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask2).exactly(1));


        // WHEN
        data = new QMimeData;
        data->setData("application/x-zanshin-object", "object");
        data->setProperty("objects", QVariant::fromValue(Domain::Artifact::List() << rootNote));
        bool result = model->dropMimeData(data, Qt::MoveAction, -1, -1, childTaskIndex);

        // THEN
        QVERIFY(!result);

        // WHEN
        auto childTask3 = Domain::Task::Ptr::create();
        auto childTask4 = Domain::Task::Ptr::create();
        taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask3).thenReturn(new FakeJob(this));
        taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask4).thenReturn(new FakeJob(this));
        data = new QMimeData;
        data->setData("application/x-zanshin-object", "object");
        data->setProperty("objects", QVariant::fromValue(Domain::Artifact::List() << childTask3 << childTask4));
        model->dropMimeData(data, Qt::MoveAction, -1, -1, rootTaskIndex);

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask3).exactly(1));
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::associate).when(rootTask, childTask4).exactly(1));
    }

    void shouldAddTasks()
    {
        // GIVEN

        // One Tag
        auto tag = Domain::Tag::Ptr::create();

        // ... in fact we won't list any model
        mock_object<Domain::TagQueries> tagQueriesMock;
        mock_object<Domain::TaskQueries> taskQueriesMock;

        // Nor create notes...
        mock_object<Domain::NoteRepository> noteRepositoryMock;

        // We'll gladly create a task though
        mock_object<Domain::TaskRepository> taskRepositoryMock;
        taskRepositoryMock(&Domain::TaskRepository::createInTag).when(any<Domain::Task::Ptr>(),
                                                                          any<Domain::Tag::Ptr>())
                                                                    .thenReturn(new FakeJob(this));

        Presentation::TagPageModel page(tag,
                                        &tagQueriesMock.getInstance(),
                                        &taskQueriesMock.getInstance(),
                                        &taskRepositoryMock.getInstance(),
                                        &noteRepositoryMock.getInstance());

        // WHEN
        page.addTask("New task");

        // THEN
        QVERIFY(taskRepositoryMock(&Domain::TaskRepository::createInTag).when(any<Domain::Task::Ptr>(),
                                                                                  any<Domain::Tag::Ptr>())
                                                                            .exactly(1));
    }
};

QTEST_MAIN(TagPageModelTest)

#include "tagpagemodeltest.moc"
