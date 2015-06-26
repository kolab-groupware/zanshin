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


#include "artifactfilterproxymodel.h"

#include <limits>

#include "domain/artifact.h"
#include "domain/task.h"

#include "presentation/querytreemodelbase.h"

using namespace Presentation;

ArtifactFilterProxyModel::ArtifactFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent),
      m_sortType(TitleSort),
      m_filterType(ShowUndone)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortOrder(Qt::AscendingOrder);
}

ArtifactFilterProxyModel::SortType ArtifactFilterProxyModel::sortType() const
{
    return m_sortType;
}

void ArtifactFilterProxyModel::setSortType(ArtifactFilterProxyModel::SortType type)
{
    m_sortType = type;
    invalidate();
}

ArtifactFilterProxyModel::FilterType ArtifactFilterProxyModel::filterType() const
{
    return m_filterType;
}

void ArtifactFilterProxyModel::setFilterType(ArtifactFilterProxyModel::FilterType type)
{
    m_filterType = type;
    invalidate();
}

void ArtifactFilterProxyModel::setSortOrder(Qt::SortOrder order)
{
    sort(0, order);
}

bool ArtifactFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto artifact = index.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();
    if (artifact) {
        QRegExp regexp = filterRegExp();
        regexp.setCaseSensitivity(Qt::CaseInsensitive);
        const bool matchesTextFilter = artifact->title().contains(regexp) || artifact->text().contains(regexp);

        bool matchesPropertyFilter = false;
        if (const auto task = artifact.objectCast<Domain::Task>()) {
            switch (m_filterType) {
                case ShowAll:
                    matchesPropertyFilter = true;
                    break;
                case ShowUndone:
                    matchesPropertyFilter = !task->isDone();
                    break;
                case ShowDone:
                    matchesPropertyFilter = task->isDone();
                    break;
            }
        } else {
            matchesPropertyFilter = true;
        }

        if (matchesTextFilter && matchesPropertyFilter) {
            return true;
        }
    }

    for (int childRow = 0; childRow < sourceModel()->rowCount(index); childRow++) {
        if (filterAcceptsRow(childRow, index))
            return true;
    }

    return false;
}

static QDateTime validDt(const QDateTime &date = QDateTime())
{
    if (date.isValid())
        return date;

    return QDateTime::fromTime_t(std::numeric_limits<uint>::max() - 1);
}

static int statusPriority(Domain::Task::Status status)
{
    switch (status) {
        case Domain::Task::Status::FullComplete:
            return 0;
        case Domain::Task::Status::Complete:
            return 0;
        case Domain::Task::Status::None:
            return 2;
        case Domain::Task::Status::NeedsAction:
            return 3;
        case Domain::Task::Status::InProcess:
            return 4;
        case Domain::Task::Status::Cancelled:
            return 1;
    };
    return -1;
}

bool ArtifactFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (m_sortType == TitleSort) {
        return QSortFilterProxyModel::lessThan(left, right);
    } else if (m_sortType == DateSort) {
        const auto leftArtifact = left.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();
        const auto rightArtifact = right.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();

        const auto leftTask = leftArtifact.objectCast<Domain::Task>();
        const auto rightTask = rightArtifact.objectCast<Domain::Task>();

        const QDateTime leftDue = leftTask ? validDt(leftTask->dueDate()) : validDt().addSecs(1);
        const QDateTime rightDue = rightTask ? validDt(rightTask->dueDate()) : validDt().addSecs(1);

        const QDateTime leftStart = leftTask ? validDt(leftTask->startDate()) : validDt().addSecs(1);
        const QDateTime rightStart = rightTask ? validDt(rightTask->startDate()) : validDt().addSecs(1);

        return leftDue < rightDue
            || leftStart < rightStart;
    } else if (m_sortType == ProgressSort) {
        const auto leftArtifact = left.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();
        const auto rightArtifact = right.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();

        const auto leftTask = leftArtifact.objectCast<Domain::Task>();
        const auto rightTask = rightArtifact.objectCast<Domain::Task>();

        const int leftProgress = leftTask ? leftTask->progress() : std::numeric_limits<int>::max();
        const int rightProgress = rightTask ? rightTask->progress() : std::numeric_limits<int>::max();

        return leftProgress < rightProgress;
    } else if (m_sortType == StatusSort) {
        const auto leftArtifact = left.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();
        const auto rightArtifact = right.data(QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();

        const auto leftTask = leftArtifact.objectCast<Domain::Task>();
        const auto rightTask = rightArtifact.objectCast<Domain::Task>();

        const int leftStatusPriority = leftTask ? statusPriority(leftTask->status()) : 0;
        const int rightStatusPriority = rightTask ? statusPriority(rightTask->status()) : 0;

        return leftStatusPriority < rightStatusPriority;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}
