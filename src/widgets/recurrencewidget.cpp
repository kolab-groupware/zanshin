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


#include "recurrencewidget.h"

#include "ui_recurrencewidget.h"
#include <KGlobal>

#include <QDebug>
#include <kcalendarsystem.h>

using namespace Widgets;

enum RecurrenceType {
  RecurrenceTypeNone = 0,
  RecurrenceTypeDaily,
  RecurrenceTypeWeekly,
  RecurrenceTypeMonthly,
  RecurrenceTypeYearly
};

enum RepeatType {
    RepeatEndless = 0,
    RepeatTime,
    RepeatCount
};

enum EveryType {
    None = -1,
    First,
    Second,
    Third,
    Fourth,
    Last,
    Every
};

class DateWidgetItem : public QListWidgetItem
{
public:
    virtual void setData(int role, const QVariant &value);
    virtual QVariant data(int role) const;
private:
    QDateTime dateTime;
};

QVariant DateWidgetItem::data(int role) const
{
    if (role == Qt::EditRole) {
        return dateTime;
    } else {
        return QListWidgetItem::data(role);
    }
}

void DateWidgetItem::setData(int role, const QVariant &value)
{
    if (role == Qt::EditRole) {
        dateTime = value.toDateTime();
    } else {
        QListWidgetItem::setData(role, value);
    }
}


RecurrenceWidget::RecurrenceWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::RecurrenceWidget)
{
    ui->setupUi(this);

    fillCombos();
    clear();

    connect( ui->mExceptionAddButton, SIGNAL(clicked()),
             SLOT(addException()));
    connect( ui->mExceptionRemoveButton, SIGNAL(clicked()),
             SLOT(removeExceptions()) );
    connect( ui->mExceptionDateEdit, SIGNAL(dateChanged(QDate)),
             SLOT(handleExceptionDateChange(QDate)) );
    connect( ui->mExceptionList, SIGNAL(itemSelectionChanged()),
             SLOT(updateRemoveExceptionButton()) );
    connect( ui->mRecurrenceTypeCombo, SIGNAL(currentIndexChanged(int)),
             SLOT(handleRecurrenceTypeChange(int)));
    connect ( ui->mWeekDayCombo, SIGNAL(checkedItemsChanged(QStringList)),
              SLOT(handleWeekDayComboChanged()));
    connect ( ui->mMonthlyEveryWeekday, SIGNAL(checkedItemsChanged(QStringList)),
              SLOT(handleWeekDayComboChanged()));
    connect ( ui->mYearlyEveryWeekday, SIGNAL(checkedItemsChanged(QStringList)),
              SLOT(handleWeekDayComboChanged()));
    connect( ui->mRecurrenceEndDate, SIGNAL(dateChanged(QDate)),
             SLOT(handleEndDateChange(QDate)));
    connect( ui->mRecurrenceEndCombo, SIGNAL(currentIndexChanged(int)),
             SLOT(handleRepeatTypeChange(int)));
    connect( ui->mEndDurationEdit, SIGNAL(valueChanged(int)),
             SLOT(handleEndAfterOccurrencesChange(int)) );
    connect( ui->mFrequencyEdit, SIGNAL(valueChanged(int)),
             SLOT(handleFrequencyChange()) );
    connect( ui->mYearlyEveryCombo, SIGNAL(currentIndexChanged(int)),
             SLOT(handleYearlyEveryComboChanged(int)));
    connect( ui->mYearlyEachCombo, SIGNAL(checkedItemsChanged(QStringList)),
             SLOT(handleYearlyEachComboChanged()));
    connect( ui->mMonthlyEachCombo, SIGNAL(checkedItemsChanged(QStringList)),
             SLOT(handleMonthlyEachComboChanged()));
    connect( ui->mYearlyEveryCombo, SIGNAL(currentIndexChanged(int)),
             SLOT(handleEveryComboChanged(int)));
    connect( ui->mMonthlyEveryCombo, SIGNAL(currentIndexChanged(int)),
             SLOT(handleEveryComboChanged(int)));
}

RecurrenceWidget::~RecurrenceWidget()
{
    delete ui;
}

void RecurrenceWidget::setStartDate(const QDateTime &date)
{
    ui->mExceptionDateEdit->setDate(date.date());
}

void RecurrenceWidget::setRecurrenceType(Domain::Recurrence::Frequency frequency)
{
    int type = RecurrenceTypeNone;
    switch (frequency){
    case Domain::Recurrence::Daily:
        type = RecurrenceTypeDaily;
        break;
    case Domain::Recurrence::Weekly:
        type = RecurrenceTypeWeekly;
        break;
    case Domain::Recurrence::Monthly:
        type = RecurrenceTypeMonthly;
        break;
    case Domain::Recurrence::Yearly:
        type = RecurrenceTypeYearly;
        break;
    default:
        qDebug() << "Unhandled recurrence type";
    };
    ui->mRecurrenceTypeCombo->setCurrentIndex(type);
    ui->mRepeatStack->setCurrentIndex(type);
    handleRecurrenceTypeChange(type);
}

void RecurrenceWidget::setRecurrenceIntervall(int intervall)
{
    ui->mFrequencyEdit->setValue(intervall);
}

void RecurrenceWidget::setExceptionDateTimes(const QList<QDateTime> &exceptionDates)
{
    ui->mExceptionList->clear();
    foreach (auto datetime, exceptionDates) {
        const QString dateStr = KGlobal::locale()->formatDate( datetime.date() );
        if(ui->mExceptionList->findItems(dateStr, Qt::MatchExactly).isEmpty()) {
            QListWidgetItem *item = new DateWidgetItem;
            item->setText(dateStr);
            item->setData(Qt::EditRole, datetime);
            ui->mExceptionList->addItem(item);
        }
    }
    updateRemoveExceptionButton();
    handleExceptionDateChange(ui->mExceptionDateEdit->date());
}

void RecurrenceWidget::setEnd(const QDateTime &end)
{
    ui->mRecurrenceEndDate->setDate(end.date());
    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatTime);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatTime);
}

void RecurrenceWidget::setEnd(int count)
{
    ui->mEndDurationEdit->setValue(count);
    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatCount);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatCount);
}

void RecurrenceWidget::setNoEnd()
{
    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatEndless);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatEndless);
}

void RecurrenceWidget::clear()
{
    ui->mRecurrenceTypeCombo->setCurrentIndex(RecurrenceTypeNone);
    ui->mRepeatStack->setCurrentIndex(RecurrenceTypeNone);
    toggleRecurrenceWidgets(RecurrenceTypeNone);

    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatEndless);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatEndless);
    ui->mEndDurationEdit->setValue(1);
    ui->mRecurrenceEndDate->setDate(QDate::currentDate());
    handleEndAfterOccurrencesChange(1);

    ui->mExceptionAddButton->setEnabled(true);
    ui->mExceptionList->clear();
    updateRemoveExceptionButton();

    QBitArray days(7, 0);
    ui->mWeekDayCombo->setDays(days);
    ui->mMonthlyEveryWeekday->setDays(days);
    ui->mYearlyEveryWeekday->setDays(days);

    ui->mMonthlyEachRadio->setChecked(true);
    ui->mMonthlyEveryCombo->setCurrentIndex(0);
    ui->mYearlyEveryCombo->setCurrentIndex(0);
    ui->mYearlyEveryWeekday->setEnabled(false);

    ui->mMonthlyEachCombo->setCheckedItems(QStringList());
    ui->mYearlyEachCombo->setCheckedItems(QStringList());
}

void RecurrenceWidget::setByDay(const QList< Domain::Recurrence::Weekday > &dayList)
{
    QBitArray days(7, 0);
    foreach(auto day, dayList) {
        days.setBit(day - Domain::Recurrence::Monday);
    }

    ui->mWeekDayCombo->setDays(days);
    ui->mMonthlyEveryWeekday->setDays(days);
    ui->mYearlyEveryWeekday->setDays(days);

    if (dayList.isEmpty()) {
        ui->mYearlyEveryCombo->setCurrentIndex(0);
    } else {
        ui->mMonthlyEveryRadio->setChecked(true);
    }
}

void RecurrenceWidget::setByDayPosition(const Domain::Recurrence::WeekPosition weekPosition)
{
    EveryType type;
    switch(weekPosition) {
    case Domain::Recurrence::WeekPosition::First:
        type = First;
        break;
    case Domain::Recurrence::WeekPosition::Second:
        type = Second;
        break;
    case Domain::Recurrence::WeekPosition::Third:
        type = Third;
        break;
    case Domain::Recurrence::WeekPosition::Fourth:
        type = Fourth;
        break;
    case Domain::Recurrence::WeekPosition::Last:
        type = Last;
        break;
    default:
        type = Every;
    }

    for(int i= ui->mYearlyEveryCombo->model()->rowCount()-1; i> -1; i--) {
        if (ui->mYearlyEveryCombo->itemData(i, Qt::UserRole).toInt() == type) {
            ui->mYearlyEveryCombo->setCurrentIndex(i);
            break;
        }
    }

    for(int i= ui->mMonthlyEveryCombo->model()->rowCount()-1; i> -1; i--) {
        if (ui->mMonthlyEveryCombo->itemData(i, Qt::UserRole).toInt() == type) {
            ui->mMonthlyEveryCombo->setCurrentIndex(i);
            break;
        }
    }
}

void RecurrenceWidget::setByMonth(const QList< int > &monthList)
{
    for(int i= ui->mYearlyEachCombo->model()->rowCount()-1; i> -1; i--) {
        if (monthList.contains(ui->mYearlyEachCombo->itemData(i, Qt::UserRole).toInt())) {
            ui->mYearlyEachCombo->setItemCheckState(i,Qt::Checked);
        } else {
         ui->mYearlyEachCombo->setItemCheckState(i,Qt::Unchecked);
        }
    }
}

void RecurrenceWidget::setByMonthDay(const QList< int > &dayList)
{
    for(int i= ui->mMonthlyEachCombo->model()->rowCount()-1; i> -1; i--) {
        if (dayList.contains(ui->mMonthlyEachCombo->itemData(i, Qt::UserRole).toInt())) {
            ui->mMonthlyEachCombo->setItemCheckState(i,Qt::Checked);
        } else {
         ui->mMonthlyEachCombo->setItemCheckState(i,Qt::Unchecked);
        }
    }
    if (!dayList.isEmpty()) {
        ui->mMonthlyEachRadio->setChecked(true);
    }
}

void RecurrenceWidget::addException()
{
    const QDate date = ui->mExceptionDateEdit->date();
    if (!date.isValid()) {
        qWarning() << "Refusing to add invalid date";
        return;
    }

    const QString dateStr = KGlobal::locale()->formatDate( date );
    if(ui->mExceptionList->findItems(dateStr, Qt::MatchExactly).isEmpty()) {
        QListWidgetItem *item = new DateWidgetItem;
        item->setText(dateStr);
        item->setData(Qt::EditRole, QDateTime(date));
        ui->mExceptionList->addItem(item);
    }

    ui->mExceptionAddButton->setEnabled(false);
    emitExceptionDatesChanged();
}

void RecurrenceWidget::handleEndAfterOccurrencesChange( int currentValue )
{
  ui->mRecurrenceOccurrencesLabel->setText(i18ncp("Recurrence ends after n occurrences", "occurrence", "occurrences", currentValue));
  emit endChanged(currentValue);
}

void RecurrenceWidget::handleEndDateChange(const QDate &date)
{
    emit endChanged(QDateTime(date));
}

void RecurrenceWidget::handleRepeatTypeChange(int currentIndex)
{
    switch(currentIndex) {
    case RepeatCount:
        emit endChanged(ui->mEndDurationEdit->value());
        break;
    case RepeatTime:
        emit endChanged(QDateTime(ui->mRecurrenceEndDate->date()));
        break;
    case RepeatEndless:
        emit noEnd();
        break;
    default:
        qWarning() << "Unknown repeat type" << currentIndex;
    }
}

void RecurrenceWidget::handleExceptionDateChange( const QDate &currentDate )
{
  const QDate date = ui->mExceptionDateEdit->date();
  const QString dateStr = KGlobal::locale()->formatDate( date );

  ui->mExceptionAddButton->setEnabled( ui->mExceptionList->findItems( dateStr, Qt::MatchExactly ).isEmpty());
}

void RecurrenceWidget::handleFrequencyChange()
{
    handleRecurrenceTypeChange(ui->mRecurrenceTypeCombo->currentIndex());
}

void RecurrenceWidget::handleRecurrenceTypeChange(int currentIndex)
{
  toggleRecurrenceWidgets(currentIndex);
  QString labelFreq;
  QString freqKey;
  int frequency = ui->mFrequencyEdit->value();
  switch (currentIndex) {
  case 2:
    labelFreq = i18ncp("repeat every N >weeks<", "week", "weeks", frequency);
    freqKey = 'w';
    break;
  case 3:
    labelFreq = i18ncp("repeat every N >months<", "month", "months", frequency);
    freqKey = 'm';
    break;
  case 4:
    labelFreq = i18ncp("repeat every N >years<", "year", "years", frequency);
    freqKey = 'y';
    break;
  default:
    labelFreq = i18ncp("repeat every N >days<", "day", "days", frequency);
    freqKey = 'd';
  }

  QString labelEvery;
  labelEvery = ki18ncp("repeat >every< N years/months/...; "
                        "dynamic context 'type': 'd' days, 'w' weeks, "
                        "'m' months, 'y' years",
                        "every", "every").
               subs( frequency ).inContext( "type", freqKey ).toString();
  ui->mFrequencyLabel->setText( labelEvery );
  ui->mRecurrenceRuleLabel->setText( labelFreq );

  emitFrequencyChanged();
}

void RecurrenceWidget::handleWeekDayComboChanged()
{
    QList <Domain::Recurrence::Weekday> days;
    QBitArray dayBits = static_cast<KPIM::KWeekdayCheckCombo*>(QObject::sender())->days();
    for ( int i = 0; i < 7; ++i ) {
        if (dayBits.testBit(i)) {
            days.append((Domain::Recurrence::Weekday) (Domain::Recurrence::Monday + i));
        }
    }

    emit byDayChanged(days);
}

void RecurrenceWidget::handleYearlyEachComboChanged()
{
    QList<int> months;
    foreach (auto month, ui->mYearlyEachCombo->checkedItems(Qt::UserRole)) {
        months << month.toInt();
    }

    emit byMonthChanged(months);
}

void RecurrenceWidget::handleEveryComboChanged(int currentIndex)
{
    QComboBox *sender = static_cast<QComboBox*>(QObject::sender());
    switch(sender->itemData(currentIndex).toInt()) {
    case First:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::First);
        break;
    case Second:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::Second);
        break;
    case Third:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::Third);
        break;
    case Fourth:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::Fourth);
        break;
    case Last:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::Last);
        break;
    case Every:
        emit byDayPositionChanged(Domain::Recurrence::WeekPosition::All);
        break;
    case None:
    default:
        emit byDayChanged(QList<Domain::Recurrence::Weekday>());
    }
}

void RecurrenceWidget::handleYearlyEveryComboChanged(int currentIndex)
{
    ui->mYearlyEveryWeekday->setEnabled( ui->mYearlyEveryCombo->itemData(currentIndex).toInt() == None ? false : true);
}

void RecurrenceWidget::handleMonthlyEachComboChanged()
{
    QList<int> days;
    foreach (auto day, ui->mMonthlyEachCombo->checkedItems(Qt::UserRole)) {
        days << day.toInt();
    }

    emit byMonthDaysChanged(days);
}

void RecurrenceWidget::removeExceptions()
{
  QList<QListWidgetItem *> selectedExceptions = ui->mExceptionList->selectedItems();
  foreach (QListWidgetItem *selectedException, selectedExceptions) {
    const int row = ui->mExceptionList->row(selectedException);
    delete ui->mExceptionList->takeItem(row);
  }

  handleExceptionDateChange(ui->mExceptionDateEdit->date());
  emitExceptionDatesChanged();
}

void RecurrenceWidget::updateRemoveExceptionButton()
{
  ui->mExceptionRemoveButton->setEnabled(ui->mExceptionList->selectedItems().count() > 0);
}

void RecurrenceWidget::emitExceptionDatesChanged()
{
  QList<QDateTime> dates;
  for(int i=0; i< ui->mExceptionList->count(); i++) {
      QListWidgetItem *item = ui->mExceptionList->item(i);
      dates.append(item->data(Qt::EditRole).value<QDateTime>());
  }
  emit exceptionDatesChanged(dates);
}

void RecurrenceWidget::emitFrequencyChanged()
{
    Domain::Recurrence::Frequency frequency = Domain::Recurrence::None;
    switch(ui->mRecurrenceTypeCombo->currentIndex()) {
    case RecurrenceTypeDaily:
        frequency = Domain::Recurrence::Daily;
        break;
    case RecurrenceTypeWeekly:
        frequency = Domain::Recurrence::Weekly;
        break;
    case RecurrenceTypeMonthly:
        frequency = Domain::Recurrence::Monthly;
        break;
    case RecurrenceTypeYearly:
        frequency = Domain::Recurrence::Yearly;
        break;
    }
    emit frequencyChanged(frequency, ui->mFrequencyEdit->value());
}

void RecurrenceWidget::toggleRecurrenceWidgets(int currentIndex)
{
    bool enable = (currentIndex != RecurrenceTypeNone);
    ui->mFrequencyLabel->setVisible(enable);
    ui->mFrequencyEdit->setVisible(enable);
    ui->mRecurrenceRuleLabel->setVisible(enable);
    ui->mRecurrenceEndLabel->setVisible(enable);
    ui->mOnLabel->setVisible(enable && currentIndex != RecurrenceTypeDaily);
    ui->mRepeatStack->setVisible(enable && currentIndex != RecurrenceTypeDaily);
    ui->mRecurrenceEndCombo->setVisible( enable );
    ui->mRecurrenceEndStack->setVisible( enable );

    // Exceptions widgets
    ui->mExceptionsLabel->setVisible( enable );
    ui->mExceptionDateEdit->setVisible( enable );
    ui->mExceptionAddButton->setVisible( enable );
    ui->mExceptionRemoveButton->setVisible( enable );
    ui->mExceptionList->setVisible( enable );
}

void RecurrenceWidget::fillCombos()
{
    const KCalendarSystem *calSys = KGlobal::locale()->calendar();

    ui->mMonthlyEachCombo->clear();
    ui->mYearlyEachCombo->clear();
    ui->mMonthlyEveryCombo->clear();
    ui->mYearlyEveryCombo->clear();

    ui->mMonthlyEachCombo->blockSignals(true);
    ui->mYearlyEachCombo->blockSignals(true);
    ui->mMonthlyEveryCombo->blockSignals(true);
    ui->mYearlyEveryCombo->blockSignals(true);

    for(int i=1; i < 32; i++) {
        ui->mMonthlyEachCombo->addItem(QString::number(i), i);
    }

    int year = QDate::currentDate().year();
    for(int i=1; i < 13; i++) {
        ui->mYearlyEachCombo->addItem(calSys->monthName(i, year), i);
    }

    ui->mYearlyEveryCombo->addItem(i18nc("no every recurrence on weekday","None"),None);
    for(int i=0; i < 6; i++) {
        QString every;
        switch(i) {
        case First:
            every = i18nc("every first week of month or year", "first");
            break;
        case Second:
            every = i18nc("every second week of month or year", "second");
            break;
        case Third:
            every = i18nc("every third week of month or year", "third");
            break;
        case Fourth:
            every = i18nc("every fourth week of month or year", "fourth");
            break;
        case Last:
            every = i18nc("every last week of month or year", "last");
            break;
        case Every:
            every = i18nc("every week of month or year", "every");
            break;
        }

        ui->mYearlyEveryCombo->addItem(every, i);
        ui->mMonthlyEveryCombo->addItem(every, i);
    }

    ui->mMonthlyEachCombo->blockSignals(false);
    ui->mYearlyEachCombo->blockSignals(false);
    ui->mMonthlyEveryCombo->blockSignals(false);
    ui->mYearlyEveryCombo->blockSignals(false);
}

