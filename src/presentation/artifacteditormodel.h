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


#ifndef PRESENTATION_ARTIFACTEDITORMODEL_H
#define PRESENTATION_ARTIFACTEDITORMODEL_H

#include <QDateTime>
#include <QObject>

#include "domain/task.h"

class QTimer;

namespace Domain {
    class NoteRepository;
    class TaskRepository;
}

namespace Presentation {

class ArtifactEditorModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Domain::Artifact::Ptr artifact READ artifact WRITE setArtifact NOTIFY artifactChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QDateTime startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDateTime dueDate READ dueDate WRITE setDueDate NOTIFY dueDateChanged)
    Q_PROPERTY(QString delegateText READ delegateText NOTIFY delegateTextChanged)
    Q_PROPERTY(bool hasTaskProperties READ hasTaskProperties NOTIFY hasTaskPropertiesChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(int status READ status WRITE setStatus NOTIFY statusChanged)
public:
    explicit ArtifactEditorModel(Domain::TaskRepository *taskRepository,
                                 Domain::NoteRepository *noteRepository,
                                 QObject *parent = 0);
    ~ArtifactEditorModel();

    Domain::Artifact::Ptr artifact() const;
    void setArtifact(const Domain::Artifact::Ptr &artifact);

    bool hasTaskProperties() const;

    QString text() const;
    QString title() const;
    QDateTime startDate() const;
    QDateTime dueDate() const;
    QString delegateText() const;
    int progress() const;
    int status() const;

    static int autoSaveDelay();

public slots:
    void setText(const QString &text);
    void setTitle(const QString &title);
    void setStartDate(const QDateTime &start);
    void setDueDate(const QDateTime &due);
    void setDelegate(const QString &name, const QString &email);
    void delegate(const QString &name, const QString &email);
    void setProgress(int progress);
    void setStatus(int status);

signals:
    void artifactChanged(const Domain::Artifact::Ptr &artifact);
    void hasTaskPropertiesChanged(bool hasTaskProperties);
    void textChanged(const QString &text);
    void titleChanged(const QString &title);
    void startDateChanged(const QDateTime &date);
    void dueDateChanged(const QDateTime &due);
    void delegateTextChanged(const QString &delegateText);
    void progressChanged(int progress);
    void statusChanged(int status);

private slots:
    void onTextChanged(const QString &text);
    void onTitleChanged(const QString &title);
    void onStartDateChanged(const QDateTime &start);
    void onDueDateChanged(const QDateTime &due);
    void onDelegateChanged(const Domain::Task::Delegate &delegate);
    void onProgressChanged(int status);
    void onStatusChanged(int status);

    void save();

private:
    void setSaveNeeded(bool needed);
    bool isSaveNeeded() const;

    Domain::TaskRepository *m_taskRepository;
    Domain::NoteRepository *m_noteRepository;

    Domain::Artifact::Ptr m_artifact;

    QString m_text;
    QString m_title;
    QDateTime m_start;
    QDateTime m_due;
    Domain::Task::Delegate m_delegate;
    int m_progress;
    Domain::Task::Status m_status;

    QTimer *m_saveTimer;
    bool m_saveNeeded;
};

}

#endif // PRESENTATION_ARTIFACTEDITORMODEL_H
