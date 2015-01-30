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


#ifndef DOMAIN_TASK_H
#define DOMAIN_TASK_H

#include "artifact.h"
#include <QDateTime>

namespace Domain {

class Recurrence : public QObject
{
    Q_OBJECT
public:
    typedef QSharedPointer<Recurrence> Ptr;
    typedef QList<Recurrence::Ptr> List;

    enum Frequency {
        None,
        Yearly,
        Monthly,
        Weekly,
        Daily,
        Hourly,
        Minutely,
        Secondly
    };

    enum Weekday {
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday,
        Saturday,
        Sunday
    };

    explicit Recurrence(QObject *parent = 0);
    explicit Recurrence(const Domain::Recurrence &other);
    virtual ~Recurrence();

    Recurrence &operator=(const Recurrence &other);
    bool operator==(const Recurrence &other) const;

    void setFrequency(Frequency);
    Frequency frequency() const;

    void setWeekStart(Weekday);
    Weekday weekStart() const;

    void setAllDay(bool);
    bool allDay() const;

    void setEnd(const QDateTime &);
    QDateTime end() const;

    void setCount(int count);
    int count() const;

    void setInterval(int interval);
    int interval() const;

    void setBysecond(const QList<int> &);
    QList<int> bysecond() const;

    void setByminute(const QList<int> &);
    QList<int> byminute() const;

    void setByhour(const QList<int> &);
    QList<int> byhour() const;

    void setByday(const QList<Weekday> &);
    QList<Weekday> byday() const;

    void setBymonthday(const QList<int> &);
    QList<int> bymonthday() const;

    void setByyearday(const QList<int> &);
    QList<int> byyearday() const;

    void setByweekno(const QList<int> &);
    QList<int> byweekno() const;

    void setBymonth(const QList<int> &);
    QList<int> bymonth() const;

    void setRecurrenceDates(const QList<QDateTime> &rdates);
    QList<QDateTime> recurrenceDates() const;

    void setExceptionDates(const QList<QDateTime> &exceptions);
    QList<QDateTime> exceptionDates() const;

private:
    Frequency m_frequency;
    Weekday m_weekStart;
    bool m_allDay;
    QDateTime m_end;
    int m_count;
    int m_interval;
    QList<int> m_bysecond;
    QList<int> m_byminute;
    QList<int> m_byhour;
    QList<Weekday> m_byday;
    QList<int> m_bymonthday;
    QList<int> m_byyearday;
    QList<int> m_byweekno;
    QList<int> m_bymonth;
    QList<QDateTime> m_recurrenceDates;
    QList<QDateTime> m_exceptionDates;
};

class Task : public Artifact
{
    Q_OBJECT
    Q_PROPERTY(QDateTime startDate READ startDate WRITE setStartDate NOTIFY startDateChanged)
    Q_PROPERTY(QDateTime dueDate READ dueDate WRITE setDueDate NOTIFY dueDateChanged)
    Q_PROPERTY(Domain::Task::Delegate delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(Domain::Recurrence::Ptr recurrence READ recurrence WRITE setRecurrence NOTIFY recurrenceChanged)
public:
    typedef QSharedPointer<Task> Ptr;
    typedef QList<Task::Ptr> List;

    enum Status {
        None,
        NeedsAction,
        InProcess,
        Complete,
        Cancelled
    };

    class Delegate
    {
    public:
        Delegate();
        Delegate(const QString &name, const QString &email);
        Delegate(const Delegate &other);

        Delegate &operator=(const Delegate &other);
        bool operator==(const Delegate &other) const;

        bool isValid() const;
        QString display() const;

        QString name() const;
        void setName(const QString &name);

        QString email() const;
        void setEmail(const QString &email);

    private:
        QString m_name;
        QString m_email;
    };

    explicit Task(QObject *parent = 0);
    virtual ~Task();

    bool isDone() const;
    QDateTime startDate() const;
    QDateTime dueDate() const;
    Delegate delegate() const;
    int progress() const;
    Status status() const;
    Recurrence::Ptr recurrence() const;

public slots:
    void setDone(bool done);
    void setStartDate(const QDateTime &startDate);
    void setDueDate(const QDateTime &dueDate);
    void setDelegate(const Domain::Task::Delegate &delegate);
    void setProgress(int progress);
    void setStatus(int status);
    void setRecurrence(const Domain::Recurrence::Ptr &recurrence);

signals:
    void startDateChanged(const QDateTime &startDate);
    void dueDateChanged(const QDateTime &dueDate);
    void delegateChanged(const Domain::Task::Delegate &delegate);
    void progressChanged(int progress);
    void statusChanged(int status);
    void recurrenceChanged(const Domain::Recurrence::Ptr &recurrence);

private:
    QDateTime m_startDate;
    QDateTime m_dueDate;
    Delegate m_delegate;
    Recurrence::Ptr m_recurrence;
    int m_progress;
    Status m_status;
};

}

Q_DECLARE_METATYPE(Domain::Task::Ptr)
Q_DECLARE_METATYPE(Domain::Task::List)
Q_DECLARE_METATYPE(Domain::Task::Delegate)

Q_DECLARE_METATYPE(Domain::Recurrence::Ptr)
Q_DECLARE_METATYPE(Domain::Recurrence::List)
#endif // DOMAIN_TASK_H
