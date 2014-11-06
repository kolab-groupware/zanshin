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

#include "domain/task.h"
#include "domain/taskrepository.h"
#include "domain/note.h"
#include "domain/noterepository.h"

using namespace Presentation;

ArtifactEditorModel::ArtifactEditorModel(Domain::TaskRepository *taskRepository,
                                         Domain::NoteRepository *noteRepository,
                                         QObject *parent)
    : QObject(parent),
      m_taskRepository(taskRepository),
      m_noteRepository(noteRepository),
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
    }

    emit textChanged(m_text);
    emit titleChanged(m_title);
    emit startDateChanged(m_start);
    emit dueDateChanged(m_due);
    emit delegateTextChanged(m_delegate.display());
    emit hasTaskPropertiesChanged(hasTaskProperties());
    emit artifactChanged(m_artifact);
    emit progressChanged(m_progress);
    emit statusChanged(m_status);
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
