/* This file is part of Zanshin

   Copyright 2015 Sandro Knauß <knauss@kolabsys.com>

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


#ifndef WIDGETS_RECURRENCEWIDGET_H
#define WIDGETS_RECURRENCEWIDGET_H

#include <QWidget>
#include <QDate>
#include <domain/task.h>

namespace Ui {
    class RecurrenceWidget;
}

namespace Widgets {

class RecurrenceWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RecurrenceWidget(QWidget *parent = 0);
    virtual ~RecurrenceWidget();

public slots:
    void setStartDate(const QDateTime &date);
    void setRecurrenceType(Domain::Recurrence::Frequency frequency);
    void setRecurrenceIntervall(int intervall);
    void setExceptionDateTimes(const QList<QDateTime> &exceptionDates);
    void setEnd(const QDateTime &end);
    void setEnd(int count);
    void setNoEnd();
    void clear();
    void setByDay(const QList<Domain::Recurrence::Weekday> &dayList);

signals:
    void frequencyChanged(Domain::Recurrence::Frequency frequency, int intervall);
    void endChanged(QDateTime endDate);
    void endChanged(int count);
    void noEnd();
    void exceptionDatesChanged(const QList<QDateTime> &exceptionDates);
    void byDayChanged(const QList<Domain::Recurrence::Weekday> &dayList);

private slots:
    void handleExceptionDateChange(const QDate &date);
    void handleFrequencyChange();
    void handleEndAfterOccurrencesChange(int);
    void handleRecurrenceTypeChange(int);
    void updateRemoveExceptionButton();
    void removeExceptions();
    void addException();
    void handleEndDateChange(const QDate &date);
    void handleRepeatTypeChange(int);
    void handleWeekDayComboChanged();

private:
    void toggleRecurrenceWidgets(int currentIndex);
    void fillCombos();
    void emitFrequencyChanged();
    void emitExceptionDatesChanged();

    Ui::RecurrenceWidget *ui;
};

}

#endif // WIDGETS_RECURRENCEWIDGET_H
