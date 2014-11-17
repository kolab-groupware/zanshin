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
#include <QSortFilterProxyModel>
#include <QStringListModel>
#include <QTreeView>

#include <KLineEdit>

#include "presentation/metatypes.h"

#include "widgets/availablesourcesview.h"
#include "widgets/datasourcedelegate.h"

class AvailableSourcesModelStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString searchTerm READ searchTerm WRITE setSearchTerm)
public:
    explicit AvailableSourcesModelStub(QObject *parent = 0)
        : QObject(parent)
    {
    }

    QString searchTerm() const
    {
        return m_searchTerm;
    }

public slots:
    void setSearchTerm(const QString &term)
    {
        m_searchTerm = term;
    }

    void listSource(const Domain::DataSource::Ptr &source)
    {
        listedSources << source;
    }

    void unlistSource(const Domain::DataSource::Ptr &source)
    {
        unlistedSources << source;
    }

    void bookmarkSource(const Domain::DataSource::Ptr &source)
    {
        bookmarkedSources << source;
    }

public:
    QList<Domain::DataSource::Ptr> listedSources;
    QList<Domain::DataSource::Ptr> unlistedSources;
    QList<Domain::DataSource::Ptr> bookmarkedSources;

private:
    QString m_searchTerm;
};


class AvailableSourcesViewTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveDefaultState()
    {
        Widgets::AvailableSourcesView available;

        QVERIFY(!available.model());

        auto sourcesView = available.findChild<QTreeView*>("sourcesView");
        QVERIFY(sourcesView);
        QVERIFY(sourcesView->isVisibleTo(&available));
        QVERIFY(!sourcesView->header()->isVisibleTo(&available));
        QVERIFY(qobject_cast<Widgets::DataSourceDelegate*>(sourcesView->itemDelegate()));

        auto searchEdit = available.findChild<KLineEdit*>("searchEdit");
        QVERIFY(searchEdit);
        QVERIFY(searchEdit->isVisibleTo(&available));
        QVERIFY(searchEdit->isClearButtonShown());
        QCOMPARE(searchEdit->clickMessage(), tr("Search..."));

        auto proxy = qobject_cast<QSortFilterProxyModel*>(sourcesView->model());
        QVERIFY(proxy);
        QVERIFY(proxy->dynamicSortFilter());
        QCOMPARE(proxy->sortColumn(), 0);
        QCOMPARE(proxy->sortOrder(), Qt::AscendingOrder);
    }

    void shouldDisplayListFromPageModel()
    {
        // GIVEN
        QStringListModel model(QStringList() << "A" << "B" << "C" );

        QObject stubPagesModel;
        stubPagesModel.setProperty("sourceListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::AvailableSourcesView available;
        auto sourcesView = available.findChild<QTreeView*>("sourcesView");
        QVERIFY(sourcesView);
        auto proxy = qobject_cast<QSortFilterProxyModel*>(sourcesView->model());
        QVERIFY(proxy);
        QVERIFY(!proxy->sourceModel());

        // WHEN
        available.setModel(&stubPagesModel);
        QTest::qWait(10);

        // THEN
        QCOMPARE(proxy->sourceModel(), &model);
    }

    void shouldListASourceWhenTheDelegateButtonIsClicked()
    {
        // GIVEN
        auto source = Domain::DataSource::Ptr::create();

        QStringListModel model(QStringList() << "A" << "B" << "C" );
        AvailableSourcesModelStub stubPagesModel;
        stubPagesModel.setProperty("sourceListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::AvailableSourcesView available;
        auto sourcesDelegate = available.findChild<Widgets::DataSourceDelegate*>();
        QVERIFY(sourcesDelegate);
        available.setModel(&stubPagesModel);
        QTest::qWait(10);

        // WHEN
        QVERIFY(QMetaObject::invokeMethod(sourcesDelegate, "actionTriggered",
                                          Q_ARG(Domain::DataSource::Ptr, source),
                                          Q_ARG(int, Widgets::DataSourceDelegate::AddToList)));

        // THEN
        QCOMPARE(stubPagesModel.listedSources.size(), 1);
        QCOMPARE(stubPagesModel.listedSources.first(), source);
    }

    void shouldUnlistASourceWhenTheDelegateButtonIsClicked()
    {
        // GIVEN
        auto source = Domain::DataSource::Ptr::create();

        QStringListModel model(QStringList() << "A" << "B" << "C" );
        AvailableSourcesModelStub stubPagesModel;
        stubPagesModel.setProperty("sourceListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::AvailableSourcesView available;
        auto sourcesDelegate = available.findChild<Widgets::DataSourceDelegate*>();
        QVERIFY(sourcesDelegate);
        available.setModel(&stubPagesModel);
        QTest::qWait(10);

        // WHEN
        QVERIFY(QMetaObject::invokeMethod(sourcesDelegate, "actionTriggered",
                                          Q_ARG(Domain::DataSource::Ptr, source),
                                          Q_ARG(int, Widgets::DataSourceDelegate::RemoveFromList)));

        // THEN
        QCOMPARE(stubPagesModel.unlistedSources.size(), 1);
        QCOMPARE(stubPagesModel.unlistedSources.first(), source);
    }

    void shouldBookmarkASourceWhenTheDelegateButtonIsClicked()
    {
        // GIVEN
        auto source = Domain::DataSource::Ptr::create();

        QStringListModel model(QStringList() << "A" << "B" << "C" );
        AvailableSourcesModelStub stubPagesModel;
        stubPagesModel.setProperty("sourceListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::AvailableSourcesView available;
        auto sourcesDelegate = available.findChild<Widgets::DataSourceDelegate*>();
        QVERIFY(sourcesDelegate);
        available.setModel(&stubPagesModel);
        QTest::qWait(10);

        // WHEN
        QVERIFY(QMetaObject::invokeMethod(sourcesDelegate, "actionTriggered",
                                          Q_ARG(Domain::DataSource::Ptr, source),
                                          Q_ARG(int, Widgets::DataSourceDelegate::Bookmark)));

        // THEN
        QCOMPARE(stubPagesModel.bookmarkedSources.size(), 1);
        QCOMPARE(stubPagesModel.bookmarkedSources.first(), source);
    }

    void shouldSwitchToSearchListWhenASearchTermIsGiven()
    {
        // GIVEN
        QStringListModel sourceModel(QStringList() << "A" << "B" << "C" );
        QStringListModel searchModel(QStringList() << "D" << "E" << "F" );

        AvailableSourcesModelStub stubPagesModel;
        stubPagesModel.setProperty("sourceListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&sourceModel)));
        stubPagesModel.setProperty("searchListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&searchModel)));

        Widgets::AvailableSourcesView available;
        auto sourcesView = available.findChild<QTreeView*>("sourcesView");
        QVERIFY(sourcesView);
        auto proxy = qobject_cast<QSortFilterProxyModel*>(sourcesView->model());
        QVERIFY(proxy);
        available.setModel(&stubPagesModel);
        QCOMPARE(proxy->sourceModel(), &sourceModel);

        auto searchEdit = available.findChild<QLineEdit*>("searchEdit");
        QVERIFY(searchEdit);

        // WHEN
        QTest::keyClick(searchEdit, 'm');

        // THEN
        QCOMPARE(proxy->sourceModel(), &sourceModel);
        QVERIFY(stubPagesModel.searchTerm().isEmpty());

        // WHEN
        QTest::keyClick(searchEdit, 'y');

        // THEN
        QCOMPARE(proxy->sourceModel(), &sourceModel);
        QVERIFY(stubPagesModel.searchTerm().isEmpty());

        // WHEN
        QTest::keyClick(searchEdit, ' ');

        // THEN
        QCOMPARE(proxy->sourceModel(), &searchModel);
        QCOMPARE(stubPagesModel.searchTerm(), QString("my "));

        // WHEN
        QTest::keyClicks(searchEdit, "term");

        // THEN
        QCOMPARE(proxy->sourceModel(), &searchModel);
        QCOMPARE(stubPagesModel.searchTerm(), QString("my term"));

        // WHEN
        searchEdit->clear();

        // THEN
        QCOMPARE(proxy->sourceModel(), &sourceModel);
        QVERIFY(stubPagesModel.searchTerm().isEmpty());
    }
};

QTEST_MAIN(AvailableSourcesViewTest)

#include "availablesourcesviewtest.moc"
