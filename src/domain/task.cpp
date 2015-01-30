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


#include "task.h"
#include <KDebug>

using namespace Domain;

Task::Task(QObject *parent)
    : Artifact(parent),
    m_progress(0),
    m_status(None)
{

}

Task::~Task()
{
}

bool Task::isDone() const
{
    return (status() == Complete);
}

void Task::setDone(bool done)
{
    if (done) {
        setStatus(Complete);
    } else {
        setStatus(None);
    }
}

QDateTime Task::startDate() const
{
    return m_startDate;
}

void Task::setStartDate(const QDateTime &startDate)
{
    if (m_startDate == startDate)
        return;

    m_startDate = startDate;
    emit startDateChanged(startDate);
}

QDateTime Task::dueDate() const
{
    return m_dueDate;
}

Task::Delegate Task::delegate() const
{
    return m_delegate;
}

void Task::setDueDate(const QDateTime &dueDate)
{
    if (m_dueDate == dueDate)
        return;

    m_dueDate = dueDate;
    emit dueDateChanged(dueDate);
}

void Task::setDelegate(const Task::Delegate &delegate)
{
    if (m_delegate == delegate)
        return;

    m_delegate = delegate;
    emit delegateChanged(delegate);
}

int Task::progress() const
{
    return m_progress;
}

void Task::setProgress(int progress)
{
    if (m_progress == progress)
        return;

    m_progress = progress;
    emit progressChanged(progress);
}

Task::Status Task::status() const
{
    return m_status;
}

void Task::setStatus(int status)
{
    if (m_status == static_cast<Status>(status))
        return;

    m_status = static_cast<Status>(status);
    emit statusChanged(status);
}

Recurrence::Ptr Task::recurrence() const
{
    return m_recurrence;
}

void Task::setRecurrence(const Domain::Recurrence::Ptr &recurrence)
{
    if (m_recurrence == recurrence) {
        return;
    }

    m_recurrence = recurrence;
    emit recurrenceChanged(recurrence);
}


Task::Delegate::Delegate()
{
}

Task::Delegate::Delegate(const QString &name, const QString &email)
    : m_name(name), m_email(email)
{
}

Task::Delegate::Delegate(const Task::Delegate &other)
    : m_name(other.m_name), m_email(other.m_email)
{
}

Task::Delegate &Task::Delegate::operator=(const Task::Delegate &other)
{
    Delegate copy(other);
    std::swap(m_name, copy.m_name);
    std::swap(m_email, copy.m_email);
    return *this;
}

bool Task::Delegate::operator==(const Task::Delegate &other) const
{
    return m_name == other.m_name
        && m_email == other.m_email;
}

bool Task::Delegate::isValid() const
{
    return !m_email.isEmpty();
}

QString Task::Delegate::display() const
{
    return !isValid() ? QString()
         : !m_name.isEmpty() ? m_name
         : m_email;
}

QString Task::Delegate::name() const
{
    return m_name;
}

void Task::Delegate::setName(const QString &name)
{
    m_name = name;
}

QString Task::Delegate::email() const
{
    return m_email;
}

void Task::Delegate::setEmail(const QString &email)
{
    m_email = email;
}

Recurrence::Recurrence(QObject *parent)
    : QObject(parent)
{

}

Recurrence::Recurrence(const Recurrence &recurrence)
    : QObject(recurrence.parent())
{
    this->m_frequency = recurrence.m_frequency;
    this->m_weekStart = recurrence.m_weekStart;
    this->m_allDay = recurrence.m_allDay;
    this->m_end = recurrence.m_end;
    this->m_count = recurrence.m_count;
    this->m_interval = recurrence.m_interval;
    this->m_bysecond = recurrence.m_bysecond;
    this->m_byminute = recurrence.m_byminute;
    this->m_byhour = recurrence.m_byhour;
    this->m_byday = recurrence.m_byday;
    this->m_bymonthday = recurrence.m_bymonthday;
    this->m_byyearday = recurrence.m_byyearday;
    this->m_byweekno = recurrence.m_byweekno;
    this->m_bymonth = recurrence.m_bymonth;
    this->m_recurrenceDates = recurrence.m_recurrenceDates;
    this->m_exceptionDates = recurrence.m_exceptionDates;
}

Recurrence::~Recurrence()
{

}

void Recurrence::setFrequency(Recurrence::Frequency frequency)
{
    m_frequency = frequency;
}

Recurrence::Frequency Recurrence::frequency() const
{
    return m_frequency;
}

void Recurrence::setWeekStart(Recurrence::Weekday weekStart)
{
    m_weekStart = weekStart;
}

Recurrence::Weekday Recurrence::weekStart() const
{
    return m_weekStart;
}

void Recurrence::setAllDay(bool allDay)
{
    m_allDay = allDay;
}

bool Recurrence::allDay() const
{
    return m_allDay;
}

void Recurrence::setEnd(const QDateTime &end)
{
    m_end = end;
}

QDateTime Recurrence::end() const
{
    return m_end;
}

void Recurrence::setCount(int count)
{
    m_count = count;
}

int Recurrence::count() const
{
    return m_count;
}

void Recurrence::setInterval(int interval)
{
    m_interval = interval;
}

int Recurrence::interval() const
{
    return m_interval;
}

void Recurrence::setBysecond(const QList<int> &bysecond)
{
    m_bysecond = bysecond;
}

QList<int> Recurrence::bysecond() const
{
    return m_bysecond;
}

void Recurrence::setByminute(const QList<int> &byminute)
{
    m_byminute = byminute;
}

QList<int> Recurrence::byminute() const
{
    return m_byminute;
}

void Recurrence::setByhour(const QList<int> &byhour)
{
    m_byhour = byhour;
}

QList<int> Recurrence::byhour() const
{
    return m_byhour;
}

void Recurrence::setByday(const QList<Recurrence::Weekday> &byday)
{
    m_byday = byday;
}

QList<Recurrence::Weekday> Recurrence::byday() const
{
    return m_byday;
}

void Recurrence::setBymonthday(const QList<int> &bymonthday)
{
    m_bymonthday = bymonthday;
}

QList<int> Recurrence::bymonthday() const
{
    return m_bymonthday;
}

void Recurrence::setByyearday(const QList<int> &byyearday)
{
    m_byyearday = byyearday;
}

QList<int> Recurrence::byyearday() const
{
    return m_byyearday;
}

void Recurrence::setByweekno(const QList<int> &byweekno)
{
    m_byweekno = byweekno;
}

QList<int> Recurrence::byweekno() const
{
    return m_byweekno;
}

void Recurrence::setBymonth(const QList<int> &bymonth)
{
    m_bymonth = bymonth;
}

QList<int> Recurrence::bymonth() const
{
    return m_bymonth;
}

void Recurrence::setRecurrenceDates(const QList<QDateTime> &recurrenceDates)
{
    m_recurrenceDates = recurrenceDates;
}

QList<QDateTime> Recurrence::recurrenceDates() const
{
    return m_recurrenceDates;
}

void Recurrence::setExceptionDates(const QList<QDateTime> &exceptionDates)
{
    m_exceptionDates = exceptionDates;
}

QList<QDateTime> Recurrence::exceptionDates() const
{
    return m_exceptionDates;
}
