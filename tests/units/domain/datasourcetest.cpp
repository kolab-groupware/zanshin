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

#include "domain/datasource.h"

using namespace Domain;

class DataSourceTest : public QObject
{
    Q_OBJECT
public:
    explicit DataSourceTest(QObject *parent = 0)
        : QObject(parent)
    {
        qRegisterMetaType<DataSource::ContentTypes>();
        qRegisterMetaType<DataSource::ListStatus>();
    }

private slots:
    void shouldHaveEmptyPropertiesByDefault()
    {
        DataSource ds;
        QCOMPARE(ds.name(), QString());
        QCOMPARE(ds.iconName(), QString());
        QCOMPARE(ds.contentTypes(), DataSource::NoContent);
        QCOMPARE(ds.listStatus(), DataSource::Unlisted);
        QVERIFY(!ds.isSelected());
    }

    void shouldNotifyNameChanges()
    {
        DataSource ds;
        QSignalSpy spy(&ds, SIGNAL(nameChanged(QString)));
        ds.setName("Foo");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), QString("Foo"));
    }

    void shouldNotNotifyIdenticalNameChanges()
    {
        DataSource ds;
        ds.setName("Foo");
        QSignalSpy spy(&ds, SIGNAL(nameChanged(QString)));
        ds.setName("Foo");
        QCOMPARE(spy.count(), 0);
    }

    void shouldNotifyIconNameChanges()
    {
        DataSource ds;
        QSignalSpy spy(&ds, SIGNAL(iconNameChanged(QString)));
        ds.setIconName("Foo");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().toString(), QString("Foo"));
    }

    void shouldNotNotifyIdenticalIconNameChanges()
    {
        DataSource ds;
        ds.setIconName("Foo");
        QSignalSpy spy(&ds, SIGNAL(iconNameChanged(QString)));
        ds.setIconName("Foo");
        QCOMPARE(spy.count(), 0);
    }

    void shouldNotifyContentTypesChanges()
    {
        DataSource ds;
        QSignalSpy spy(&ds, SIGNAL(contentTypesChanged(Domain::DataSource::ContentTypes)));
        ds.setContentTypes(Domain::DataSource::Notes);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().value<Domain::DataSource::ContentTypes>(),
                 Domain::DataSource::Notes);
    }

    void shouldNotNotifyIdenticalContentTypesChanges()
    {
        DataSource ds;
        ds.setContentTypes(Domain::DataSource::Notes);
        QSignalSpy spy(&ds, SIGNAL(contentTypesChanged(Domain::DataSource::ContentTypes)));
        ds.setContentTypes(Domain::DataSource::Notes);
        QCOMPARE(spy.count(), 0);
    }

    void shouldNotifySelectedChanges()
    {
        DataSource ds;
        QSignalSpy spy(&ds, SIGNAL(selectedChanged(bool)));
        ds.setSelected(true);
        QCOMPARE(spy.count(), 1);
        QVERIFY(spy.first().first().toBool());
    }

    void shouldNotNotifyIdenticalSelectedChanges()
    {
        DataSource ds;
        ds.setSelected(true);
        QSignalSpy spy(&ds, SIGNAL(selectedChanged(bool)));
        ds.setSelected(true);
        QCOMPARE(spy.count(), 0);
    }

    void shouldNotifyListStatusChanges()
    {
        DataSource ds;
        QSignalSpy spy(&ds, SIGNAL(listStatusChanged(Domain::DataSource::ListStatus)));
        ds.setListStatus(DataSource::Bookmarked);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.first().first().value<DataSource::ListStatus>(), DataSource::Bookmarked);
    }

    void shouldNotNotifyIdenticalListStatusChanges()
    {
        DataSource ds;
        ds.setListStatus(DataSource::Bookmarked);
        QSignalSpy spy(&ds, SIGNAL(listStatusChanged(Domain::DataSource::ListStatus)));
        ds.setListStatus(DataSource::Bookmarked);
        QCOMPARE(spy.count(), 0);
    }
};

QTEST_MAIN(DataSourceTest)

#include "datasourcetest.moc"
