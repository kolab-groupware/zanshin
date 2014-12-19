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


#ifndef DOMAIN_QUERYRESULTPROVIDER_H
#define DOMAIN_QUERYRESULTPROVIDER_H

#include <algorithm>
#include <functional>

#include <QList>
#include <QSharedPointer>

namespace Domain {

template<typename ItemType>
class QueryResultProvider;

template<typename InputType>
class QueryResultInputImpl
{
public:
    typedef typename QueryResultProvider<InputType>::Ptr ProviderPtr;
    typedef QSharedPointer<QueryResultInputImpl<InputType> > Ptr;
    typedef QWeakPointer<QueryResultInputImpl<InputType>> WeakPtr;
    typedef std::function<void(InputType, int)> ChangeHandler;
    typedef QList<ChangeHandler> ChangeHandlerList;

    virtual ~QueryResultInputImpl() {}

protected:
    explicit QueryResultInputImpl(const ProviderPtr &provider)
        : m_provider(provider)
    {
    }

    static void registerResult(const ProviderPtr &provider, const Ptr &result)
    {
        provider->m_results << result;
    }

    static ProviderPtr retrieveProvider(const Ptr &result)
    {
        return result->m_provider;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList preInsertHandlers() const
    {
        return m_preInsertHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList postInsertHandlers() const
    {
        return m_postInsertHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList preRemoveHandlers() const
    {
        return m_preRemoveHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList postRemoveHandlers() const
    {
        return m_postRemoveHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList preReplaceHandlers() const
    {
        return m_preReplaceHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList postReplaceHandlers() const
    {
        return m_postReplaceHandlers;
    }

    // cppcheck can't figure out the friend class
    // cppcheck-suppress unusedPrivateFunction
    ChangeHandlerList doneHandlers() const
    {
        return m_doneHandlers;
    }

    friend class QueryResultProvider<InputType>;
    ProviderPtr m_provider;
    ChangeHandlerList m_preInsertHandlers;
    ChangeHandlerList m_postInsertHandlers;
    ChangeHandlerList m_preRemoveHandlers;
    ChangeHandlerList m_postRemoveHandlers;
    ChangeHandlerList m_preReplaceHandlers;
    ChangeHandlerList m_postReplaceHandlers;
    ChangeHandlerList m_doneHandlers;
};

template<typename ItemType>
class QueryResultProvider
{
public:
    typedef QSharedPointer<QueryResultProvider<ItemType>> Ptr;
    typedef QWeakPointer<QueryResultProvider<ItemType>> WeakPtr;

    typedef QSharedPointer<QueryResultInputImpl<ItemType> > ResultPtr;
    typedef QWeakPointer<QueryResultInputImpl<ItemType>> ResultWeakPtr;
    typedef std::function<void(ItemType, int)> ChangeHandler;
    typedef QList<ChangeHandler> ChangeHandlerList;


    QueryResultProvider()
    {
    }

    QList<ItemType> data() const
    {
        return m_list;
    }

    void append(const ItemType &item)
    {
        cleanupResults();
        ChangeHandlerGetter preInsert = [](ResultPtr ptr) { return ptr->preInsertHandlers(); };
        ChangeHandlerGetter postInsert = [](ResultPtr ptr) { return ptr->postInsertHandlers(); };
        callChangeHandlers(item, m_list.size(), preInsert);
        m_list.append(item);
        callChangeHandlers(item, m_list.size()-1, postInsert);
    }

    void prepend(const ItemType &item)
    {
        cleanupResults();
        ChangeHandlerGetter preInsert = [](ResultPtr ptr) { return ptr->preInsertHandlers(); };
        ChangeHandlerGetter postInsert = [](ResultPtr ptr) { return ptr->postInsertHandlers(); };
        callChangeHandlers(item, 0, preInsert);
        m_list.prepend(item);
        callChangeHandlers(item, 0, postInsert);
    }

    void done()
    {
        cleanupResults();
        ChangeHandlerGetter done = [](ResultPtr ptr) { return ptr->doneHandlers(); };
        callChangeHandlers(ItemType(), 0, done);
    }

    void insert(int index, const ItemType &item)
    {
        cleanupResults();
        ChangeHandlerGetter preInsert = [](ResultPtr ptr) { return ptr->preInsertHandlers(); };
        ChangeHandlerGetter postInsert = [](ResultPtr ptr) { return ptr->postInsertHandlers(); };
        callChangeHandlers(item, index, preInsert);
        m_list.insert(index, item);
        callChangeHandlers(item, index, postInsert);
    }

    ItemType takeFirst()
    {
        cleanupResults();
        ChangeHandlerGetter preRemove = [](ResultPtr ptr) { return ptr->preRemoveHandlers(); };
        ChangeHandlerGetter postRemove = [](ResultPtr ptr) { return ptr->postRemoveHandlers(); };
        const ItemType item = m_list.first();
        callChangeHandlers(item, 0, preRemove);
        m_list.removeFirst();
        callChangeHandlers(item, 0, postRemove);
        return item;
    }

    void removeFirst()
    {
        takeFirst();
    }

    ItemType takeLast()
    {
        cleanupResults();
        ChangeHandlerGetter preRemove = [](ResultPtr ptr) { return ptr->preRemoveHandlers(); };
        ChangeHandlerGetter postRemove = [](ResultPtr ptr) { return ptr->postRemoveHandlers(); };
        const ItemType item = m_list.last();
        callChangeHandlers(item, m_list.size()-1, preRemove);
        m_list.removeLast();
        callChangeHandlers(item, m_list.size(), postRemove);
        return item;
    }

    void removeLast()
    {
        takeLast();
    }

    ItemType takeAt(int index)
    {
        cleanupResults();
        ChangeHandlerGetter preRemove = [](ResultPtr ptr) { return ptr->preRemoveHandlers(); };
        ChangeHandlerGetter postRemove = [](ResultPtr ptr) { return ptr->postRemoveHandlers(); };
        const ItemType item = m_list.at(index);
        callChangeHandlers(item, index, preRemove);
        m_list.removeAt(index);
        callChangeHandlers(item, index, postRemove);
        return item;
    }

    void removeAt(int index)
    {
        takeAt(index);
    }

    void remove(ItemType item)
    {
        const int index = m_list.indexOf(item);
        removeAt(index);
    }

    void replace(int index, const ItemType &item)
    {
        cleanupResults();
        ChangeHandlerGetter preReplace = [](ResultPtr ptr) { return ptr->preReplaceHandlers(); };
        ChangeHandlerGetter postReplace = [](ResultPtr ptr) { return ptr->postReplaceHandlers(); };
        callChangeHandlers(m_list.at(index), index, preReplace);
        m_list.replace(index, item);
        callChangeHandlers(item, index, postReplace);
    }

    QueryResultProvider &operator<< (const ItemType &item)
    {
        append(item);
        return *this;
    }

private:
    void cleanupResults()
    {
        m_results.erase(std::remove_if(m_results.begin(),
                                       m_results.end(),
                                       std::mem_fn(&QueryResultInputImpl<ItemType>::WeakPtr::isNull)),
                        m_results.end());
    }

    typedef std::function<ChangeHandlerList(ResultPtr)> ChangeHandlerGetter;

    void callChangeHandlers(const ItemType &item, int index, const ChangeHandlerGetter &handlerGetter)
    {
        for (auto weakResult : m_results)
        {
            auto result = weakResult.toStrongRef();
            if (!result) continue;
            for (auto handler : handlerGetter(result))
            {
                handler(item, index);
            }
        }
    }

    friend class QueryResultInputImpl<ItemType>;
    QList<ItemType> m_list;
    QList<ResultWeakPtr> m_results;
};

}

#endif // DOMAIN_QUERYRESULTPROVIDER_H
