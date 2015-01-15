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


#ifndef DOMAIN_RELATION_H
#define DOMAIN_RELATION_H

#include <QMetaType>
#include <QSharedPointer>
#include <QString>
#include <QUrl>

namespace Domain {

class Relation : public QObject
{
    Q_OBJECT
    // Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)

public:
    typedef QSharedPointer<Relation> Ptr;
    typedef QList<Relation::Ptr> List;

    explicit Relation(QObject *parent = 0);
    virtual ~Relation();

    QString name() const;
    QUrl url() const;

public slots:
    void setName(const QString &name);
    void setUrl(const QUrl &url);
private:
    QString m_name;
    QUrl m_url;
};

}

Q_DECLARE_METATYPE(Domain::Relation::Ptr)

#endif // DOMAIN_RELATION_H
