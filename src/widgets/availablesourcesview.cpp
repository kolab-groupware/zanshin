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


#include "availablesourcesview.h"

#include <functional>

#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>
#include <QItemSelection>

#include <KLineEdit>

#include "presentation/metatypes.h"
#include "presentation/querytreemodelbase.h"

#include "widgets/datasourcedelegate.h"

using namespace Widgets;

AvailableSourcesView::AvailableSourcesView(QWidget *parent)
    : QWidget(parent),
      m_model(0),
      m_sortProxy(new QSortFilterProxyModel(this))
{
    m_sortProxy->setDynamicSortFilter(true);
    m_sortProxy->sort(0);

    auto searchEdit = new KLineEdit(this);
    searchEdit->setObjectName("searchEdit");
    searchEdit->setClearButtonShown(true);
    searchEdit->setClickMessage(tr("Search..."));
    connect(searchEdit, SIGNAL(textChanged(QString)),
            this, SLOT(onSearchTextChanged(QString)));

    auto sourcesView = new QTreeView(this);
    sourcesView->setObjectName("sourcesView");
    sourcesView->header()->hide();
    sourcesView->setModel(m_sortProxy);
    connect(sourcesView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
            this, SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &)));
    m_selectionModel = sourcesView->selectionModel();

    auto delegate = new DataSourceDelegate(sourcesView);
    connect(delegate, SIGNAL(actionTriggered(Domain::DataSource::Ptr,int)),
            this, SLOT(onActionTriggered(Domain::DataSource::Ptr,int)));
    sourcesView->setItemDelegate(delegate);

    auto layout = new QVBoxLayout;
    layout->addWidget(searchEdit);
    layout->addWidget(sourcesView);
    setLayout(layout);

    connect(m_sortProxy, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(onRefreshDefaultSource()));
    connect(m_sortProxy, SIGNAL(layoutChanged()), this, SLOT(onRefreshDefaultSource()));
    connect(m_sortProxy, SIGNAL(modelReset()), this, SLOT(onRefreshDefaultSource()));
    connect(m_sortProxy, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(onRefreshDefaultSource()));
    connect(m_sortProxy, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SLOT(onRefreshDefaultSource()));
    onRefreshDefaultSource();
}

QObject *AvailableSourcesView::model() const
{
    return m_model;
}

void AvailableSourcesView::setModel(QObject *model)
{
    if (model == m_model)
        return;

    m_sortProxy->setSourceModel(0);

    m_model = model;

    setSourceModel("sourceListModel");
}

void AvailableSourcesView::onSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    const QModelIndexList indexes = selected.indexes();
    if (indexes.size() > 1) {
        qWarning() << "Selected multiple indexes, no idea what to do with that";
    }
    if (indexes.size() >= 1) {
        const QModelIndex &idx = indexes.first();
        const QVariant data = idx.data(Presentation::QueryTreeModelBase::ObjectRole);
        const auto source = data.value<Domain::DataSource::Ptr>();
        emit sourceActivated(source);
    }
}

void AvailableSourcesView::setDefaultSourceProperty(QObject *object, const char *property)
{
    m_object = object;
    m_property = property;
    onRefreshDefaultSource();
}

void AvailableSourcesView::onRefreshDefaultSource()
{
    if (!m_object)
        return;

    const QVariant data = m_object->property(m_property);
    const auto defaultSource = data.value<Domain::DataSource::Ptr>();

    if (!defaultSource)
        return;

    selectSource(defaultSource, QModelIndex());
}

void AvailableSourcesView::selectSource(Domain::DataSource::Ptr source, const QModelIndex &parentIndex)
{
    std::function<void(QAbstractItemModel *, const QModelIndex &, std::function<void(const QModelIndex &)>)> traverseTree;
    traverseTree = [&traverseTree](QAbstractItemModel *model, const QModelIndex &parent, std::function<void(const QModelIndex &)> visitor) {
        for (int i = 0; i < model->rowCount(parent); i++) {
            visitor(parent);
            if (model->hasChildren(parent)) {
                traverseTree(model, model->index(i, 0, parent), visitor);
            }
        }
    };
    QVariant modelProperty = m_model->property("searchListModel");
    if (modelProperty.canConvert<QAbstractItemModel*>()) {
        traverseTree(modelProperty.value<QAbstractItemModel*>(), QModelIndex(), [this, &source](const QModelIndex &index) {
            if (index.data(Presentation::QueryTreeModelBase::ObjectRole).value<Domain::DataSource::Ptr>() == source) {
                // todo select proxy
                m_selectionModel->select(index, QItemSelectionModel::SelectCurrent);
            }
        });
    }

}

void AvailableSourcesView::onActionTriggered(const Domain::DataSource::Ptr &source, int action)
{
    switch (action) {
    case DataSourceDelegate::AddToList:
        QMetaObject::invokeMethod(m_model, "listSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    case DataSourceDelegate::RemoveFromList:
        QMetaObject::invokeMethod(m_model, "unlistSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    case DataSourceDelegate::Bookmark:
        QMetaObject::invokeMethod(m_model, "bookmarkSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    default:
        qFatal("Shouldn't happen");
        break;
    }
}

void AvailableSourcesView::setSourceModel(const QByteArray &propertyName)
{
    QVariant modelProperty = m_model->property(propertyName);
    if (modelProperty.canConvert<QAbstractItemModel*>())
        m_sortProxy->setSourceModel(modelProperty.value<QAbstractItemModel*>());
}

void AvailableSourcesView::onSearchTextChanged(const QString &text)
{
    if (text.size() <= 2) {
        m_model->setProperty("searchTerm", QString());
        setSourceModel("sourceListModel");
    } else {
        m_model->setProperty("searchTerm", text);
        setSourceModel("searchListModel");
    }
}
