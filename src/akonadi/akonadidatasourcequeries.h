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

#ifndef AKONADI_DATASOURCEQUERIES_H
#define AKONADI_DATASOURCEQUERIES_H

#include <functional>

#include <QHash>

#include <KJob>

#include <Akonadi/Collection>

#include "domain/datasourcequeries.h"
#include "domain/livequery.h"
#include "akonadistorageinterface.h"

namespace Akonadi {

class Item;
class MonitorInterface;
class SerializerInterface;
class TreeQuery;
class AkonadiCollectionTreeSource;

class DataSourceQueries : public QObject, public Domain::DataSourceQueries
{
    Q_OBJECT
public:
    typedef Domain::LiveQuery<Akonadi::Collection, Domain::DataSource::Ptr> DataSourceQuery;
    typedef Domain::QueryResultProvider<Domain::DataSource::Ptr> DataSourceProvider;
    typedef Domain::QueryResult<Domain::DataSource::Ptr> DataSourceResult;

    explicit DataSourceQueries(QObject *parent = 0);
    DataSourceQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor);
    virtual ~DataSourceQueries();

    void setApplicationMode(ApplicationMode);

    DataSourceResult::Ptr findTasks() const;
    DataSourceResult::Ptr findNotes() const;
    DataSourceResult::Ptr findTopLevel() const;
    DataSourceResult::Ptr findChildren(Domain::DataSource::Ptr source) const;
    DataSourceResult::Ptr findChildrenRecursive(Domain::DataSource::Ptr source) const;

    QString searchTerm() const;
    void setSearchTerm(QString term);
    DataSourceResult::Ptr findSearchTopLevel() const;
    DataSourceResult::Ptr findSearchChildren(Domain::DataSource::Ptr source) const;

private slots:
    void onCollectionAdded(const Akonadi::Collection &collection);
    void onCollectionRemoved(const Akonadi::Collection &collection);
    void onCollectionChanged(const Akonadi::Collection &collection);

private:
    void init();
    DataSourceQuery::Ptr createDataSourceQuery();
    DataSourceResult::Ptr findSearchChildrenQuery(Domain::DataSource::Ptr source, const QSharedPointer<TreeQuery> &treeQuery) const;
    QSharedPointer<AkonadiCollectionTreeSource> findVisibleCollections() const;
    QSharedPointer<AkonadiCollectionTreeSource> findVisiblePersonCollections() const;
    QSharedPointer<AkonadiCollectionTreeSource> findSearchCollections() const;
    QSharedPointer<AkonadiCollectionTreeSource> findSearchPersonCollections() const;
    QSharedPointer<TreeQuery> getVisibleCollectionTree() const;
    QSharedPointer<TreeQuery> getVisiblePersonTree() const;
    QSharedPointer<TreeQuery> getSearchCollectionTree() const;
    QSharedPointer<TreeQuery> getSearchPersonCollectionTree() const;

    StorageInterface *m_storage;
    SerializerInterface *m_serializer;
    MonitorInterface *m_monitor;
    bool m_ownInterfaces;

    DataSourceQuery::Ptr m_findTasks;
    DataSourceQuery::Ptr m_findNotes;
    DataSourceQuery::List m_dataSourceQueries;
    QString m_searchTerm;
    QSharedPointer<TreeQuery> m_treeQuery;
    QSharedPointer<TreeQuery> m_personTreeQuery;
    QSharedPointer<TreeQuery> m_searchTreeQuery;
    QSharedPointer<TreeQuery> m_searchPersonTreeQuery;

    StorageInterface::FetchContentTypes m_fetchContentTypeFilter;
};

// This represents a reactive result set for an akonadi query, which supports being recursively queried.
// (and currently omits any other way of querying)
// The result set will automatically update itself on changes in the store (therefore the monitor)
// This is essentilally what akonadi should provide with a proper query interface.
class AkonadiCollectionTreeSource : public QObject
{
    Q_OBJECT
public:
    AkonadiCollectionTreeSource(MonitorInterface *monitor);

    void findChildren(const Akonadi::Collection &parent);

    //Defines what parts match the tree. Implement to filter monitor notifications outside of the tree.
    //The filter is recursive, meaning that if a parent is filtered, children will automatically match the filter as well.
    void setFilter(const std::function<bool(const Akonadi::Collection &)> &);

    //Defines what is initially fetched to populate the tree.
    void setCollectionFetcher(const std::function<void(const std::function<void(bool, const Akonadi::Collection::List&)> &)> &fetcher);

signals:
    void added(Akonadi::Collection, Akonadi::Collection::Id parentId);
    void removed(Akonadi::Collection, Akonadi::Collection::Id parentId);
    void changed(Akonadi::Collection, Akonadi::Collection::Id parentId);

private slots:
    void onAdded(const Akonadi::Collection &);
    void onRemoved(const Akonadi::Collection &);
    void onChanged(const Akonadi::Collection &);

private:
    Akonadi::Collection::Id id(const Akonadi::Collection &col) const;
    //Internally trigger the fetchFunction and then call the appropriate signals/callbacks
    void populate(const std::function<void()> &callback);
    QHash<Collection::Id /*parent*/, Collection::List /*children*/> m_collections;
    MonitorInterface *m_monitor;
    bool m_populated;
    bool m_populationInProgress;
    QList <std::function<void()> > m_pendingCallbacks;
    std::function<bool(const Akonadi::Collection &)> isWantedCollection;
    std::function<bool(const Akonadi::Collection &)> isToplevel;
    std::function<void(const std::function<void(bool, const Akonadi::Collection::List&)> &)> fetchCollections;
};

//Maps the signals to the appropriate query
class TreeQuery : public QObject
{
    Q_OBJECT
public:
    typedef Domain::LiveQuery<Akonadi::Collection, Domain::DataSource::Ptr> DataSourceQuery;
    typedef Domain::QueryResultProvider<Domain::DataSource::Ptr> DataSourceProvider;
    typedef Domain::QueryResult<Domain::DataSource::Ptr> DataSourceResult;

    TreeQuery(SerializerInterface *, const QSharedPointer<AkonadiCollectionTreeSource> &source);
    void reset(const QSharedPointer<AkonadiCollectionTreeSource> &source);

    DataSourceResult::Ptr findChildren(Domain::DataSource::Ptr source, const std::function<void(DataSourceQuery::Ptr, const Akonadi::Collection &root)> &setupFunction);
    void findChildren(const Akonadi::Collection &col);

private slots:
    void onAdded(const Akonadi::Collection &col, Akonadi::Collection::Id parentId);
    void onRemoved(const Akonadi::Collection &col, Akonadi::Collection::Id id);
    void onChanged(const Akonadi::Collection &col, Akonadi::Collection::Id id);

private:
    QHash<Akonadi::Entity::Id, DataSourceQuery::Ptr> m_findChildren;
    SerializerInterface *m_serializer;
    QSharedPointer<AkonadiCollectionTreeSource> m_source;
};

}

#endif // AKONADI_DATASOURCEQUERIES_H
