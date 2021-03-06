/* This file is part of Zanshin Todo.

   Copyright 2011 Kevin Ottens <ervin@kde.org>

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

#include "modelpath.h"

#include <algorithm>

using namespace Zanshin::Test;

ModelPath::ModelPath()
{
}

bool ModelPath::isEmpty()
{
    return m_path.isEmpty();
}

ModelPath::ModelPath(const C &collection)
{
    m_path << QVariant::fromValue(collection);
}

ModelPath::ModelPath(const T &todo)
{
    m_path << QVariant::fromValue(todo);
}

ModelPath::ModelPath(const Cat &context)
{
    m_path << QVariant::fromValue(context);
}

ModelPath::ModelPath(const V &virt)
{
    m_path << QVariant::fromValue(virt);
}

ModelPath::ModelPath(const G& g)
{
    m_path << QVariant::fromValue(g);
}

ModelPath::ModelPath(const C &collection1, const C &collection2)
{
    m_path << QVariant::fromValue(collection1)
           << QVariant::fromValue(collection2);
}

ModelPath::ModelPath(const C &collection, const T &todo)
{
    m_path << QVariant::fromValue(collection)
           << QVariant::fromValue(todo);
}

ModelPath::ModelPath(const Cat &context1, const Cat &context2)
{
    m_path << QVariant::fromValue(context1)
           << QVariant::fromValue(context2);
}

ModelPath::ModelPath(const Cat &context, const T &todo)
{
    m_path << QVariant::fromValue(context)
           << QVariant::fromValue(todo);
}

ModelPath::ModelPath(const G& g1, const G& g2)
{
    m_path << QVariant::fromValue(g1)
    << QVariant::fromValue(g2);
}

ModelPath::ModelPath(const G& g1, const T& todo)
{
    m_path << QVariant::fromValue(g1)
    << QVariant::fromValue(todo);
}

ModelPath::ModelPath(const ModelPath &path, const C &collection)
{
    m_path = path.m_path;
    m_path << QVariant::fromValue(collection);
}

ModelPath::ModelPath(const ModelPath &path, const T &todo)
{
    m_path = path.m_path;
    m_path << QVariant::fromValue(todo);
}

ModelPath::ModelPath(const ModelPath &path, const Cat &context)
{
    m_path = path.m_path;
    m_path << QVariant::fromValue(context);
}

ModelPath::ModelPath(const ModelPath& path, const G& g)
{
    m_path = path.m_path;
    m_path << QVariant::fromValue(g);
}


ModelPath::ModelPath(const ModelPath &path1, const ModelPath &path2)
{
    m_path = path1.m_path + path2.m_path;
}

ModelPath operator%(const C &collection1, const C &collection2)
{
    return ModelPath(collection1, collection2);
}

ModelPath operator%(const C &collection, const T &todo)
{
    return ModelPath(collection, todo);
}

ModelPath operator%(const Cat &context1, const Cat &context2)
{
    return ModelPath(context1, context2);
}

ModelPath operator%(const Cat &context, const T &todo)
{
    return ModelPath(context, todo);
}

ModelPath operator%(const G& g, const T& todo)
{
    return ModelPath(g, todo);
}

ModelPath operator%(const G& g1, const G& g2)
{
    return ModelPath(g1, g2);
}

ModelPath operator%(const ModelPath &path, const C &collection)
{
    return ModelPath(path, collection);
}

ModelPath operator%(const ModelPath &path, const T &todo)
{
    return ModelPath(path, todo);
}

ModelPath operator%(const ModelPath &path, const Cat &context)
{
    return ModelPath(path, context);
}

ModelPath operator%(const ModelPath& path, const G& g)
{
    return ModelPath(path, g);
}

ModelPath operator%(const ModelPath &path1, const ModelPath &path2)
{
    return ModelPath(path1, path2);
}

