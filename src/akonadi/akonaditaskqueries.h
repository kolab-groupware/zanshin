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

#ifndef AKONADI_TASKQUERIES_H
#define AKONADI_TASKQUERIES_H

#include <functional>

#include <QHash>
#include <Akonadi/Item>

#include "domain/livequery.h"
#include "domain/taskqueries.h"

class KJob;

namespace Akonadi {

class Item;
class MonitorInterface;
class SerializerInterface;
class StorageInterface;
class TaskTreeQuery;
class AkonadiItemSource;

class TaskQueries : public QObject, public Domain::TaskQueries
{
    Q_OBJECT
public:
    typedef Domain::LiveQuery<Akonadi::Item, Domain::Task::Ptr> TaskQuery;
    typedef Domain::QueryResultProvider<Domain::Task::Ptr> TaskProvider;
    typedef Domain::QueryResult<Domain::Task::Ptr> TaskResult;

    typedef Domain::QueryResultProvider<Domain::Context::Ptr> ContextProvider;
    typedef Domain::QueryResult<Domain::Context::Ptr> ContextResult;

    explicit TaskQueries(QObject *parent = 0);
    TaskQueries(StorageInterface *storage, SerializerInterface *serializer, MonitorInterface *monitor);
    virtual ~TaskQueries();

    TaskResult::Ptr findAll() const;
    TaskResult::Ptr findChildren(Domain::Task::Ptr task) const;
    TaskResult::Ptr findTopLevel() const;
    ContextResult::Ptr findContexts(Domain::Task::Ptr task) const;

private slots:
    void onItemAdded(const Akonadi::Item &item);
    void onItemRemoved(const Akonadi::Item &item);
    void onItemChanged(const Akonadi::Item &item);

private:
    TaskQuery::Ptr createTaskQuery();
    QSharedPointer<TaskTreeQuery> getTaskTree(const Akonadi::Collection &) const;
    QSharedPointer<AkonadiItemSource> findTaskTree(const Akonadi::Collection &) const;

    StorageInterface *m_storage;
    SerializerInterface *m_serializer;
    MonitorInterface *m_monitor;
    bool m_ownInterfaces;

    TaskQuery::Ptr m_findAll;
    QHash<Akonadi::Entity::Id, TaskQuery::Ptr> m_findChildren;
    TaskQuery::Ptr m_findTopLevel;
    TaskQuery::List m_taskQueries;
    QHash<Akonadi::Collection::Id, QSharedPointer<TaskTreeQuery> > m_treeQueries;
};

// This represents a reactive result set for an akonadi query, which supports being recursively queried.
// (and currently omits any other way of querying)
// The result set will automatically update itself on changes in the store (therefore the monitor)
// This is essentilally what akonadi should provide with a proper query interface.
class AkonadiItemSource : public QObject
{
    Q_OBJECT
public:
    AkonadiItemSource(MonitorInterface *monitor);

    void findChildren(const Akonadi::Item &parent);

    //Defines what parts match the tree. Implement to filter monitor notifications outside of the tree.
    //The filter is recursive, meaning that if a parent is filtered, children will automatically match the filter as well.
    void setFilter(const std::function<bool(const Akonadi::Item &)> &);

    //Defines what is initially fetched to populate the tree.
    void setItemFetcher(const std::function<void(const std::function<void(bool, const Akonadi::Item::List&)> &)> &fetcher);

signals:
    void added(Akonadi::Item, QString);
    void removed(Akonadi::Item, QString);
    void changed(Akonadi::Item, QString);

// private slots:
//     void onAdded(const Akonadi::Item &);
//     void onRemoved(const Akonadi::Item &);
//     void onChanged(const Akonadi::Item &);

private:
    Akonadi::Collection::Id id(const Akonadi::Collection &col) const;
    //Internally trigger the fetchFunction and then call the appropriate signals/callbacks
    void populate(const std::function<void()> &callback);
    QHash<QString /*parent*/, Item::List /*children*/> m_items;
    MonitorInterface *m_monitor;
    bool m_populated;
    bool m_populationInProgress;
    QList <std::function<void()> > m_pendingCallbacks;
    std::function<bool(const Akonadi::Item &)> isWantedItem;
    std::function<bool(const Akonadi::Collection &)> isToplevel;
    std::function<void(const std::function<void(bool, const Akonadi::Item::List&)> &)> fetchItems;
};

//Maps the signals to the appropriate query

class TaskTreeQuery : public QObject
{
    Q_OBJECT
public:
    typedef Domain::LiveQuery<Akonadi::Item, Domain::Task::Ptr> Query;
    typedef Domain::QueryResultProvider<Domain::Task::Ptr> Provider;
    typedef Domain::QueryResult<Domain::Task::Ptr> Result;

    TaskTreeQuery(SerializerInterface *, const QSharedPointer<AkonadiItemSource> &source);
    void reset(const QSharedPointer<AkonadiItemSource> &source);

    Result::Ptr findChildren(Domain::Task::Ptr source, const std::function<void(typename Query::Ptr, const Akonadi::Collection &root)> &setupFunction);
    void findChildren(const Item &parent);

private slots:
    void onAdded(const Akonadi::Item &, const QString &parent);
    void onRemoved(const Akonadi::Item &, const QString &parent);
    void onChanged(const Akonadi::Item &, const QString &parent);

private:
    QHash<QString, typename Query::Ptr> m_findChildren;
    SerializerInterface *m_serializer;
    QSharedPointer<AkonadiItemSource> m_source;
};

}

#endif // AKONADI_TASKQUERIES_H
