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

#include <QtTestGui>

#include <QAbstractItemModel>
#include <QHeaderView>
#include <QLineEdit>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QTreeView>

#include "domain/task.h"

#include "presentation/artifactfilterproxymodel.h"
#include "presentation/metatypes.h"

#include "widgets/filterwidget.h"
#include "widgets/itemdelegate.h"
#include "widgets/pageview.h"

#include "messageboxstub.h"

class PageModelStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* centralListModel READ centralListModel)
public:
    QAbstractItemModel *centralListModel()
    {
        return &itemModel;
    }

    void addItem(const QString &title, QStandardItem *parentItem = 0)
    {
        QStandardItem *item = new QStandardItem;
        item->setData(title, Qt::DisplayRole);
        if (!parentItem)
            itemModel.appendRow(item);
        else
            parentItem->appendRow(item);

        taskNames << title;
    }

    void addItems(const QStringList &list)
    {
        foreach (const QString &title, list) {
            addItem(title);
        }
    }

public slots:
    void addTask(const QString &name)
    {
        taskNames << name;
    }

    void removeItem(const QModelIndex &index)
    {
        removedIndices << index;
    }

public:
    QStringList taskNames;
    QList<QPersistentModelIndex> removedIndices;
    QStandardItemModel itemModel;
};

class PageViewTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveDefaultState()
    {
        Widgets::PageView page;

        auto centralView = page.findChild<QTreeView*>("centralView");
        QVERIFY(centralView);
        QVERIFY(centralView->isVisibleTo(&page));
        QVERIFY(!centralView->header()->isVisibleTo(&page));
        QVERIFY(qobject_cast<Widgets::ItemDelegate*>(centralView->itemDelegate()));
        QVERIFY(centralView->alternatingRowColors());
        QCOMPARE(centralView->dragDropMode(), QTreeView::DragDrop);

        auto filter = page.findChild<Widgets::FilterWidget*>("filterWidget");
        QVERIFY(filter);
        QVERIFY(filter->proxyModel());
        QCOMPARE(filter->proxyModel(), centralView->model());

        QLineEdit *quickAddEdit = page.findChild<QLineEdit*>("quickAddEdit");
        QVERIFY(quickAddEdit);
        QVERIFY(quickAddEdit->isVisibleTo(&page));
        QVERIFY(quickAddEdit->text().isEmpty());
        QCOMPARE(quickAddEdit->placeholderText(), tr("Type and press enter to add an action"));
    }

    void shouldDisplayListFromPageModel()
    {
        // GIVEN
        QStandardItemModel model;

        QObject stubPageModel;
        stubPageModel.setProperty("centralListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::PageView page;
        auto centralView = page.findChild<QTreeView*>("centralView");
        QVERIFY(centralView);
        auto proxyModel = qobject_cast<Presentation::ArtifactFilterProxyModel*>(centralView->model());
        QVERIFY(proxyModel);
        QVERIFY(!proxyModel->sourceModel());

        // WHEN
        page.setModel(&stubPageModel);

        // THEN
        QCOMPARE(proxyModel->sourceModel(), &model);
    }

    void shouldNotCrashWithNullModel()
    {
        // GIVEN
        Widgets::PageView page;
        QObject stubPageModel;
        page.setModel(&stubPageModel);

        // WHEN
        page.setModel(0);

        // THEN
        QVERIFY(!page.model());
    }

    void shouldCreateTasksWhenHittingReturn()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto quickAddEdit = page.findChild<QLineEdit*>("quickAddEdit");

        // WHEN
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)
        QTest::keyClicks(quickAddEdit, "Foo");
        QTest::keyClick(quickAddEdit, Qt::Key_Return);
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)
        QTest::keyClicks(quickAddEdit, "Bar");
        QTest::keyClick(quickAddEdit, Qt::Key_Return);
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)

        // THEN
        QCOMPARE(stubPageModel.taskNames, QStringList() << "Foo" << "Bar");
    }

    void shouldDeleteItemWhenHittingTheDeleteKey()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addItems(QStringList() << "A" << "B" << "C");
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        QTreeView *centralView = page.findChild<QTreeView*>("centralView");
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QCOMPARE(stubPageModel.removedIndices.size(), 1);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
    }

    void shouldNoteTryToDeleteIfThereIsNoSelection()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addItems(QStringList() << "A" << "B" << "C");

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        QTreeView *centralView = page.findChild<QTreeView*>("centralView");
        centralView->clearSelection();
        page.findChild<QLineEdit*>("quickAddEdit")->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(stubPageModel.removedIndices.isEmpty());
    }

    void shouldDiplayNotificationWhenHittingTheDeleteKeyOnAnItemWithChildren()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addItems(QStringList() << "A" << "B");
        QStandardItem *parentIndex = stubPageModel.itemModel.item(1, 0);
        stubPageModel.addItem("C", parentIndex);
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto msgbox = MessageBoxStub::Ptr::create();
        page.setMessageBoxInterface(msgbox);

        QTreeView *centralView = page.findChild<QTreeView*>("centralView");
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(msgbox->called());
        QCOMPARE(stubPageModel.removedIndices.size(), 1);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
    }

    void shouldDeleteItemsWhenHittingTheDeleteKey()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addItems(QStringList() << "A" << "B" << "C");
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);
        QPersistentModelIndex index2 = stubPageModel.itemModel.index(2, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto msgbox = MessageBoxStub::Ptr::create();
        page.setMessageBoxInterface(msgbox);

        QTreeView *centralView = page.findChild<QTreeView*>("centralView");
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->selectionModel()->setCurrentIndex(index2, QItemSelectionModel::Select);
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(msgbox->called());
        QCOMPARE(stubPageModel.removedIndices.size(), 2);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
        QCOMPARE(stubPageModel.removedIndices.at(1), index2);
    }
};

QTEST_MAIN(PageViewTest)

#include "pageviewtest.moc"
