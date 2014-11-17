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


#ifndef DOMAIN_DATASOURCE_H
#define DOMAIN_DATASOURCE_H

#include <QMetaType>
#include <QSharedPointer>
#include <QString>

namespace Domain {

class DataSource : public QObject
{
    Q_OBJECT
    Q_ENUMS(ContentType ListStatus)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(Domain::DataSource::ContentTypes contentTypes READ contentTypes WRITE setContentTypes NOTIFY contentTypesChanged)
    Q_PROPERTY(Domain::DataSource::ListStatus listStatus READ listStatus WRITE setListStatus NOTIFY listStatusChanged)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
public:
    typedef QSharedPointer<DataSource> Ptr;
    typedef QList<DataSource::Ptr> List;

    enum ContentType {
        NoContent = 0,
        Tasks,
        Notes
    };
    Q_DECLARE_FLAGS(ContentTypes, ContentType)

    enum ListStatus {
        Unlisted = 0,
        Listed = 1,
        Bookmarked = 3
    };

    explicit DataSource(QObject *parent = 0);
    virtual ~DataSource();

    QString name() const;
    QString iconName() const;
    ContentTypes contentTypes() const;
    ListStatus listStatus() const;
    bool isSelected() const;

public slots:
    void setName(const QString &name);
    void setIconName(const QString &iconName);
    void setContentTypes(Domain::DataSource::ContentTypes types);
    void setListStatus(Domain::DataSource::ListStatus status);
    void setSelected(bool selected);

signals:
    void nameChanged(const QString &name);
    void iconNameChanged(const QString &iconName);
    void contentTypesChanged(Domain::DataSource::ContentTypes types);
    void listStatusChanged(Domain::DataSource::ListStatus status);
    void selectedChanged(bool selected);

private:
    QString m_name;
    QString m_iconName;
    ContentTypes m_contentTypes;
    ListStatus m_listStatus;
    bool m_selected;
};

}

Q_DECLARE_METATYPE(Domain::DataSource::Ptr)
Q_DECLARE_METATYPE(Domain::DataSource::ContentTypes)
Q_DECLARE_OPERATORS_FOR_FLAGS(Domain::DataSource::ContentTypes)
Q_DECLARE_METATYPE(Domain::DataSource::ListStatus)

#endif // DOMAIN_DATASOURCE_H
