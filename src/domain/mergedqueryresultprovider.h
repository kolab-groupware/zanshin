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


#ifndef DOMAIN_MERGEDQUERYRESULTPROVIDER_H
#define DOMAIN_MERGEDQUERYRESULTPROVIDER_H

#include "queryresultprovider.h"

#include <algorithm>

#include <QList>
#include <QSharedPointer>

namespace Domain {

template<typename ItemType>
class MergedQueryResultProvider : public QueryResultProvider<ItemType>
{
public:
    typedef QSharedPointer<MergedQueryResultProvider<ItemType>> Ptr;
    typedef QSharedPointer<QueryResult<ItemType> > ResultPtr;

    MergedQueryResultProvider()
        : QueryResultProvider<ItemType>()
    {
    }

    void addQueryResult(ResultPtr result) {
        //We need to keep the pointer around
        m_inputResults << result;

        result->addPostInsertHandler([this](const ItemType &item, int){
            QueryResultProvider<ItemType>::append(item);
        });
        result->addPreRemoveHandler([this](const ItemType &item, int){
            QueryResultProvider<ItemType>::remove(item);
        });

        //FIXME we need a better replace handler
        // collectionResult->addPreReplaceHandler([mergedResultProvider](const Domain::DataSource::Ptr &source, int){
        //     mergedResultProvider->remove(source);
        // });
        // collectionResult->addPostReplaceHandler([mergedResultProvider](const Domain::DataSource::Ptr &source, int){
        //     mergedResultProvider->append(source);
        // });
    }

private:
    QList<ResultPtr> m_inputResults;
};

}

#endif // DOMAIN_QUERYRESULTPROVIDER_H
