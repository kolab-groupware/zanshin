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


#include "artifacteditormodel.h"

#include <QTimer>
#include <QDebug>

#include "domain/task.h"
#include "domain/taskrepository.h"
#include "domain/note.h"
#include "domain/noterepository.h"
#include "domain/relation.h"
#include "domain/relationqueries.h"
#include "domain/relationrepository.h"

using namespace Presentation;

ArtifactEditorModel::ArtifactEditorModel(Domain::TaskRepository *taskRepository,
                                         Domain::NoteRepository *noteRepository,
                                         Domain::RelationQueries *relationQueries,
                                         Domain::RelationRepository *relationRepository,
                                         QObject *parent)
    : QObject(parent),
      m_taskRepository(taskRepository),
      m_noteRepository(noteRepository),
      m_relationQueries(relationQueries),
      m_relationRepository(relationRepository),
      m_saveTimer(new QTimer(this)),
      m_saveNeeded(false)
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(autoSaveDelay());
    connect(m_saveTimer, SIGNAL(timeout()), this, SLOT(save()));
}

ArtifactEditorModel::~ArtifactEditorModel()
{
    save();
}
Domain::Artifact::Ptr ArtifactEditorModel::artifact() const
{
    return m_artifact;
}

void ArtifactEditorModel::setArtifact(const Domain::Artifact::Ptr &artifact)
{
    if (m_artifact == artifact)
        return;

    save();

    m_text = QString();
    m_title = QString();
    m_start = QDateTime();
    m_due = QDateTime();
    m_delegate = Domain::Task::Delegate();
    m_progress = 0;
    m_status = Domain::Task::None;
    m_relations.clear();
    m_recurrence = Domain::Recurrence::Ptr(0);

    m_artifact = artifact;

    if (m_artifact) {
        disconnect(m_artifact.data(), 0, this, 0);

        m_text = artifact->text();
        m_title = artifact->title();

        connect(m_artifact.data(), SIGNAL(textChanged(QString)),
                this, SLOT(onTextChanged(QString)));
        connect(m_artifact.data(), SIGNAL(titleChanged(QString)),
                this, SLOT(onTitleChanged(QString)));
    }

    if (auto task = artifact.objectCast<Domain::Task>()) {
        m_start = task->startDate();
        m_due = task->dueDate();
        m_delegate = task->delegate();
        m_progress = task->progress();
        m_status = task->status();
        m_recurrence = task->recurrence();

        connect(m_artifact.data(), SIGNAL(startDateChanged(QDateTime)),
                this, SLOT(onStartDateChanged(QDateTime)));
        connect(m_artifact.data(), SIGNAL(dueDateChanged(QDateTime)),
                this, SLOT(onDueDateChanged(QDateTime)));
        connect(m_artifact.data(), SIGNAL(delegateChanged(Domain::Task::Delegate)),
                this, SLOT(onDelegateChanged(Domain::Task::Delegate)));
        connect(m_artifact.data(), SIGNAL(progressChanged(int)),
                this, SLOT(onProgressChanged(int)));
        connect(m_artifact.data(), SIGNAL(statusChanged(int)),
                this, SLOT(onStatusChanged(int)));
        connect(m_artifact.data(), SIGNAL(recurrenceChanged(Domain::Recurrence::Ptr)),
                this, SLOT(onRecurrenceChanged(Domain::Recurrence::Ptr)));
    }

    auto relationQuery = m_relationQueries->findRelations(m_artifact);
    relationQuery->addPostInsertHandler([this, relationQuery](const Domain::Relation::Ptr &rel, int) {
        //hack to keep query alive
        Q_ASSERT(relationQuery);
        addRelation(rel);
    });

    emit textChanged(m_text);
    emit titleChanged(m_title);
    emit startDateChanged(m_start);
    emit dueDateChanged(m_due);
    emit delegateTextChanged(m_delegate.display());
    emit hasTaskPropertiesChanged(hasTaskProperties());
    emit artifactChanged(m_artifact);
    emit progressChanged(m_progress);
    emit statusChanged(m_status);
    emit relationsChanged(m_relations);
    emit recurrenceChanged(m_recurrence);
}

void ArtifactEditorModel::addRelation(const Domain::Relation::Ptr &relation)
{
    m_relations << relation;
    emit relationsChanged(m_relations);
}

void ArtifactEditorModel::removeRelation(const Domain::Relation::Ptr &relation)
{
    m_relations.removeAll(relation);
    emit relationsChanged(m_relations);
    m_relationRepository->remove(relation);
}

bool ArtifactEditorModel::hasTaskProperties() const
{
    return m_artifact.objectCast<Domain::Task>();
}

QString ArtifactEditorModel::text() const
{
    return m_text;
}

QString ArtifactEditorModel::title() const
{
    return m_title;
}

QDateTime ArtifactEditorModel::startDate() const
{
    return m_start;
}

QDateTime ArtifactEditorModel::dueDate() const
{
    return m_due;
}

QString ArtifactEditorModel::delegateText() const
{
    return m_delegate.display();
}

int ArtifactEditorModel::progress() const
{
    return m_progress;
}

int ArtifactEditorModel::status() const
{
    return m_status;
}

Domain::Recurrence::Ptr ArtifactEditorModel::recurrence() const
{
    return m_recurrence;
}

int ArtifactEditorModel::autoSaveDelay()
{
    return 500;
}

void ArtifactEditorModel::setText(const QString &text)
{
    if (m_text == text)
        return;
    onTextChanged(text);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setTitle(const QString &title)
{
    if (m_title == title)
        return;
    onTitleChanged(title);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setStartDate(const QDateTime &start)
{
    if (m_start == start)
        return;
    onStartDateChanged(start);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setDueDate(const QDateTime &due)
{
    if (m_due == due)
        return;
    onDueDateChanged(due);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setDelegate(const QString &name, const QString &email)
{
    if ((m_delegate.name() == name) && (m_delegate.email() == email))
        return;
    onDelegateChanged(Domain::Task::Delegate(name, email));
    setSaveNeeded(true);
}

void ArtifactEditorModel::delegate(const QString &name, const QString &email)
{
    auto task = m_artifact.objectCast<Domain::Task>();
    Q_ASSERT(task);
    auto delegate = Domain::Task::Delegate(name, email);
    m_taskRepository->delegate(task, delegate);
}

void ArtifactEditorModel::setProgress(int progress)
{
    if (m_progress == progress)
        return;
    onProgressChanged(progress);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setStatus(int status)
{
    if (m_status == status)
        return;
    onStatusChanged(status);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setRecurrence(const Domain::Recurrence::Ptr &recurrence)
{
    if (m_recurrence == recurrence ||
           (m_recurrence && recurrence && *m_recurrence == *recurrence)) {
        return;
    }
    onRecurrenceChanged(recurrence);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setExceptionDates(const QList<QDateTime> &exceptionDates)
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->exceptionDates() != exceptionDates) {
        recurrence->setExceptionDates(exceptionDates);
        setRecurrence(recurrence);
    }
}

void ArtifactEditorModel::setFrequency(Domain::Recurrence::Frequency frequency, int intervall)
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->frequency() != frequency || recurrence->interval() != intervall) {
        if (frequency == Domain::Recurrence::None) {
            setRecurrence(Domain::Recurrence::Ptr(0));
        } else {
            recurrence->setFrequency(frequency);
            recurrence->setInterval(intervall);
            switch(frequency) {
            case Domain::Recurrence::Yearly:
                recurrence->setBysecond(QList<int>());
                recurrence->setByminute(QList<int>());
                recurrence->setByhour(QList<int>());
                recurrence->setByday(QList<Domain::Recurrence::Weekday>());
                recurrence->setBymonthday(QList<int>());
                recurrence->setByyearday(QList<int>());
                recurrence->setByweekno(QList<int>());
                break;
            case Domain::Recurrence::Monthly:
                recurrence->setBysecond(QList<int>());
                recurrence->setByminute(QList<int>());
                recurrence->setByhour(QList<int>());
                recurrence->setByday(QList<Domain::Recurrence::Weekday>());
                recurrence->setBymonthday(QList<int>());
                recurrence->setByyearday(QList<int>());
                recurrence->setBymonth(QList<int>());
                break;
            case Domain::Recurrence::Weekly:
                recurrence->setBysecond(QList<int>());
                recurrence->setByminute(QList<int>());
                recurrence->setByhour(QList<int>());
                recurrence->setBymonthday(QList<int>());
                recurrence->setByyearday(QList<int>());
                recurrence->setByweekno(QList<int>());
                recurrence->setBymonth(QList<int>());
                break;
            case Domain::Recurrence::Daily:
                recurrence->setBysecond(QList<int>());
                recurrence->setByminute(QList<int>());
                recurrence->setByhour(QList<int>());
                recurrence->setByday(QList<Domain::Recurrence::Weekday>());
                recurrence->setBymonthday(QList<int>());
                recurrence->setByyearday(QList<int>());
                recurrence->setByweekno(QList<int>());
                recurrence->setBymonth(QList<int>());
                break;
           }
            setRecurrence(recurrence);
        }
    }
}

void ArtifactEditorModel::setNoRepeat()
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->count() != 0  || recurrence->end().isValid()) {
        recurrence->setCount(0);
        recurrence->setEnd(QDateTime());
        setRecurrence(recurrence);
    }
}

void ArtifactEditorModel::setRepeatEnd(QDateTime endDate)
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->end().date() != endDate.date() || recurrence->count() != 0 ) {
        recurrence->setEnd(endDate);
        recurrence->setCount(0);
        setRecurrence(recurrence);
    }
}

void ArtifactEditorModel::setRepeatEnd(int count)
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->count() != count || recurrence->end().isValid()) {
        recurrence->setCount(count);
        recurrence->setEnd(QDateTime());
        setRecurrence(recurrence);
    }
}

void ArtifactEditorModel::setRepeatEndless()
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->count() != -1 || recurrence->end().isValid()) {
        recurrence->setCount(-1);
        recurrence->setEnd(QDateTime());
        setRecurrence(recurrence);
    }
}

void ArtifactEditorModel::setByDay(const QList<Domain::Recurrence::Weekday> &dayList)
{
    Domain::Recurrence::Ptr recurrence(new Domain::Recurrence);
    if (m_recurrence) {
        *recurrence = *m_recurrence;
    }

    if (!m_recurrence || recurrence->byday() != dayList) {
        recurrence->setByday(dayList);
        setRecurrence(recurrence);
    }
}


QList<Domain::Relation::Ptr> ArtifactEditorModel::relations() const
{
    return m_relations;
}

void ArtifactEditorModel::onTextChanged(const QString &text)
{
    m_text = text;
    emit textChanged(m_text);
}

void ArtifactEditorModel::onTitleChanged(const QString &title)
{
    m_title = title;
    emit titleChanged(m_title);
}

void ArtifactEditorModel::onStartDateChanged(const QDateTime &start)
{
    m_start = start;
    emit startDateChanged(m_start);
}

void ArtifactEditorModel::onDueDateChanged(const QDateTime &due)
{
    m_due = due;
    emit dueDateChanged(m_due);
}

void ArtifactEditorModel::onDelegateChanged(const Domain::Task::Delegate &delegate)
{
    m_delegate = delegate;
    emit delegateTextChanged(m_delegate.display());
}

void ArtifactEditorModel::onProgressChanged(int progress)
{
    m_progress = progress;
    emit progressChanged(progress);
}

void ArtifactEditorModel::onStatusChanged(int status)
{
    m_status = static_cast<Domain::Task::Status>(status);
    emit statusChanged(status);
}

void ArtifactEditorModel::onRecurrenceChanged(const Domain::Recurrence::Ptr &recurrence)
{
    m_recurrence = recurrence;
    emit recurrenceChanged(recurrence);
}

void ArtifactEditorModel::save()
{
    if (!isSaveNeeded())
        return;

    Q_ASSERT(m_artifact);

    m_artifact->setTitle(m_title);
    m_artifact->setText(m_text);

    if (auto task = m_artifact.objectCast<Domain::Task>()) {
        task->setStartDate(m_start);
        task->setDueDate(m_due);
        task->setDelegate(m_delegate);
        task->setProgress(m_progress);
        task->setStatus(m_status);
        task->setRecurrence(m_recurrence);
        m_taskRepository->update(task);
    } else {
        auto note = m_artifact.objectCast<Domain::Note>();
        Q_ASSERT(note);
        m_noteRepository->save(note);
    }

    setSaveNeeded(false);
}

void ArtifactEditorModel::setSaveNeeded(bool needed)
{
    if (needed)
        m_saveTimer->start();
    else
        m_saveTimer->stop();

    m_saveNeeded = needed;
}

bool ArtifactEditorModel::isSaveNeeded() const
{
    return m_saveNeeded;
}
