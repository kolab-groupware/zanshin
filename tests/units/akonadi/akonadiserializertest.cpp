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

#include "akonadi/akonadiserializer.h"

#include <Akonadi/Item>
#include <KCalCore/Todo>

class AkonadiSerializerTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldCreateTaskFromItem_data()
    {
        QTest::addColumn<QString>("summary");
        QTest::addColumn<QString>("content");
        QTest::addColumn<bool>("isDone");
        QTest::addColumn<QDateTime>("startDate");
        QTest::addColumn<QDateTime>("dueDate");

        QTest::newRow("nominal case") << "summary" << "content" << false << QDateTime(QDate(2013, 11, 24)) << QDateTime(QDate(2014, 03, 01));
        QTest::newRow("done case") << "summary" << "content" << true << QDateTime(QDate(2013, 11, 24)) << QDateTime(QDate(2014, 03, 01));
        QTest::newRow("empty case") << QString() << QString() << false << QDateTime() << QDateTime();
    }

    void shouldCreateTaskFromItem()
    {
        // GIVEN

        // Data...
        QFETCH(QString, summary);
        QFETCH(QString, content);
        QFETCH(bool, isDone);
        QFETCH(QDateTime, startDate);
        QFETCH(QDateTime, dueDate);

        // ... stored in a todo...
        KCalCore::Todo::Ptr todo(new KCalCore::Todo);
        todo->setSummary(summary);
        todo->setDescription(content);
        todo->setCompleted(isDone);
        todo->setDtStart(KDateTime(startDate));
        todo->setDtDue(KDateTime(dueDate));

        // ... as payload of an item
        Akonadi::Item item;
        item.setMimeType("application/x-vnd.akonadi.calendar.todo");
        item.setPayload<KCalCore::Todo::Ptr>(todo);

        // WHEN
        Akonadi::Serializer serializer;
        Domain::Task::Ptr task = serializer.createTaskFromItem(item);

        // THEN
        QCOMPARE(task->title(), summary);
        QCOMPARE(task->text(), content);
        QCOMPARE(task->isDone(), isDone);
        QCOMPARE(task->startDate(), startDate);
        QCOMPARE(task->dueDate(), dueDate);
    }

    void shouldCreateNullTaskFromInvalidItem()
    {
        // GIVEN
        Akonadi::Item item;

        // WHEN
        Akonadi::Serializer serializer;
        Domain::Task::Ptr task = serializer.createTaskFromItem(item);

        // THEN
        QVERIFY(task.isNull());
    }

    void shouldUpdateTaskFromItem_data()
    {
        QTest::addColumn<QString>("updatedSummary");
        QTest::addColumn<QString>("updatedContent");
        QTest::addColumn<bool>("updatedDone");
        QTest::addColumn<QDateTime>("updatedStartDate");
        QTest::addColumn<QDateTime>("updatedDueDate");

        QTest::newRow("no change") << "summary" << "content" << false << QDateTime(QDate(2013, 11, 24)) << QDateTime(QDate(2014, 03, 01));
        QTest::newRow("changed") << "new summary" << "new content" << true << QDateTime(QDate(2013, 11, 25)) << QDateTime(QDate(2014, 03, 02));
    }

    void shouldUpdateTaskFromItem()
    {
        // GIVEN

        // A todo...
        KCalCore::Todo::Ptr originalTodo(new KCalCore::Todo);
        originalTodo->setSummary("summary");
        originalTodo->setDescription("content");
        originalTodo->setCompleted(false);
        originalTodo->setDtStart(KDateTime(QDate(2013, 11, 24)));
        originalTodo->setDtDue(KDateTime(QDate(2014, 03, 01)));

        // ... as payload of an item...
        Akonadi::Item originalItem;
        originalItem.setMimeType("application/x-vnd.akonadi.calendar.todo");
        originalItem.setPayload<KCalCore::Todo::Ptr>(originalTodo);

        // ... deserialized as a task
        Akonadi::Serializer serializer;
        auto task = serializer.createTaskFromItem(originalItem);

        // WHEN

        // Data...
        QFETCH(QString, updatedSummary);
        QFETCH(QString, updatedContent);
        QFETCH(bool, updatedDone);
        QFETCH(QDateTime, updatedStartDate);
        QFETCH(QDateTime, updatedDueDate);

        // ... in a new todo...
        KCalCore::Todo::Ptr updatedTodo(new KCalCore::Todo);
        updatedTodo->setSummary(updatedSummary);
        updatedTodo->setDescription(updatedContent);
        updatedTodo->setCompleted(updatedDone);
        updatedTodo->setDtStart(KDateTime(updatedStartDate));
        updatedTodo->setDtDue(KDateTime(updatedDueDate));

        // ... as payload of a new item
        Akonadi::Item updatedItem;
        updatedItem.setMimeType("application/x-vnd.akonadi.calendar.todo");
        updatedItem.setPayload<KCalCore::Todo::Ptr>(updatedTodo);

        serializer.updateTaskFromItem(task, updatedItem);

        // THEN
        QCOMPARE(task->title(), updatedSummary);
        QCOMPARE(task->text(), updatedContent);
        QCOMPARE(task->isDone(), updatedDone);
        QCOMPARE(task->startDate(), updatedStartDate);
        QCOMPARE(task->dueDate(), updatedDueDate);
    }

    void shouldNotUpdateTaskFromInvalidItem()
    {
        // GIVEN

        // Data...
        const QString summary = "summary";
        const QString content = "content";
        const bool isDone = true;
        const QDateTime startDate(QDate(2013, 11, 24));
        const QDateTime dueDate(QDate(2014, 03, 01));

        // ... stored in a todo...
        KCalCore::Todo::Ptr originalTodo(new KCalCore::Todo);
        originalTodo->setSummary(summary);
        originalTodo->setDescription(content);
        originalTodo->setCompleted(isDone);
        originalTodo->setDtStart(KDateTime(startDate));
        originalTodo->setDtDue(KDateTime(dueDate));

        // ... as payload of an item...
        Akonadi::Item originalItem;
        originalItem.setMimeType("application/x-vnd.akonadi.calendar.todo");
        originalItem.setPayload<KCalCore::Todo::Ptr>(originalTodo);

        // ... deserialized as a task
        Akonadi::Serializer serializer;
        auto task = serializer.createTaskFromItem(originalItem);

        // WHEN
        Akonadi::Item invalidItem;
        serializer.updateTaskFromItem(task, invalidItem);

        // THEN
        QCOMPARE(task->title(), summary);
        QCOMPARE(task->text(), content);
        QCOMPARE(task->isDone(), isDone);
        QCOMPARE(task->startDate(), startDate);
        QCOMPARE(task->dueDate(), dueDate);
    }
};

QTEST_MAIN(AkonadiSerializerTest)

#include "akonadiserializertest.moc"