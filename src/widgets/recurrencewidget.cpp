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
    RepeatNone = 0,
    RepeatTime,
    RepeatCount
};

RecurrenceWidget::RecurrenceWidget(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::RecurrenceWidget)
{
    ui->setupUi(this);

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
    connect( ui->mEndDurationEdit, SIGNAL(valueChanged(int)),
             SLOT(handleEndAfterOccurrencesChange(int)) );
    connect( ui->mFrequencyEdit, SIGNAL(valueChanged(int)),
             SLOT(handleFrequencyChange()) );


}

RecurrenceWidget::~RecurrenceWidget()
{
    delete ui;
}

void RecurrenceWidget::setStartDate(const QDateTime &date)
{
    fillCombos();
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
            ui->mExceptionList->addItem(dateStr);
        }
    }
}

void RecurrenceWidget::setEnd(const QDateTime &end)
{
    qDebug() << "set enddate:" << end.date();
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
    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatNone);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatNone);
}

void RecurrenceWidget::clear()
{
    ui->mRecurrenceTypeCombo->setCurrentIndex(RecurrenceTypeNone);
    ui->mRepeatStack->setCurrentIndex(RecurrenceTypeNone);
    toggleRecurrenceWidgets(RecurrenceTypeNone);

    ui->mRecurrenceEndCombo->setCurrentIndex(RepeatNone);
    ui->mRecurrenceEndStack->setCurrentIndex(RepeatNone);
    ui->mEndDurationEdit->setValue(1);
    handleEndAfterOccurrencesChange(1);

    ui->mExceptionAddButton->setEnabled(true);
    ui->mExceptionList->clear();
    fillCombos();
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
        ui->mExceptionList->addItem(dateStr);
    }

    ui->mExceptionAddButton->setEnabled(false);
}

void RecurrenceWidget::handleEndAfterOccurrencesChange( int currentValue )
{
  ui->mRecurrenceOccurrencesLabel->setText(i18ncp("Recurrence ends after n occurrences", "occurrence", "occurrences", currentValue));
}

void RecurrenceWidget::handleExceptionDateChange( const QDate &currentDate )
{
  const QDate date = ui->mExceptionDateEdit->date();
  const QString dateStr = KGlobal::locale()->formatDate( date );

  ui->mExceptionAddButton->setEnabled(
//    currentDate >= mDateTime->startDate() &&
    ui->mExceptionList->findItems( dateStr, Qt::MatchExactly ).isEmpty());
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

  //emit recurrenceChanged( static_cast<RecurrenceType>( currentIndex ) );
}

void RecurrenceWidget::removeExceptions()
{
  QList<QListWidgetItem *> selectedExceptions = ui->mExceptionList->selectedItems();
  foreach (QListWidgetItem *selectedException, selectedExceptions) {
    const int row = ui->mExceptionList->row(selectedException);
    delete ui->mExceptionList->takeItem(row);
  }

  handleExceptionDateChange(ui->mExceptionDateEdit->date());
}

void RecurrenceWidget::updateRemoveExceptionButton()
{
  ui->mExceptionRemoveButton->setEnabled(ui->mExceptionList->selectedItems().count() > 0);
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
/*
  const KCalendarSystem *calSys = KGlobal::locale()->calendar();
  // Next the monthly combo. This contains the following elements:
  // - nth day of the month
  // - (month.lastDay() - n)th day of the month
  // - the ith ${weekday} of the month
  // - the (month.weekCount() - i)th day of the month
  const int currentMonthlyIndex = ui->mMonthlyCombo->currentIndex();
  ui->mMonthlyCombo->clear();
  const QDate date = mDateTime->startDate();

  QString item = subsOrdinal(
    ki18nc( "example: the 30th", "the %1" ), dayOfMonthFromStart() ).toString();
  ui->mMonthlyCombo->addItem( item );

  item = subsOrdinal( ki18nc( "example: the 4th to last day",
                              "the %1 to last day" ), dayOfMonthFromEnd() ).toString();
  ui->mMonthlyCombo->addItem( item );

  item = subsOrdinal(
    ki18nc( "example: the 5th Wednesday", "the %1 %2" ), monthWeekFromStart() ).
         subs(
           calSys->weekDayName( date.dayOfWeek(), KCalendarSystem::LongDayName ) ).toString();
  ui->mMonthlyCombo->addItem( item );

  if ( monthWeekFromEnd() == 1 ) {
    item = ki18nc( "example: the last Wednesday", "the last %1" ).
           subs( calSys->weekDayName(
                   date.dayOfWeek(), KCalendarSystem::LongDayName ) ).toString();
  } else {
    item = subsOrdinal(
      ki18nc( "example: the 5th to last Wednesday", "the %1 to last %2" ), monthWeekFromEnd() ).
           subs( calSys->weekDayName(
                   date.dayOfWeek(), KCalendarSystem::LongDayName ) ).toString();
  }
  ui->mMonthlyCombo->addItem( item );
  ui->mMonthlyCombo->setCurrentIndex( currentMonthlyIndex == -1 ? 0 : currentMonthlyIndex );

  // Finally the yearly combo. This contains the following options:
  // - ${n}th of ${long-month-name}
  // - ${month.lastDay() - n}th last day of ${long-month-name}
  // - the ${i}th ${weekday} of ${long-month-name}
  // - the ${month.weekCount() - i}th day of ${long-month-name}
  // - the ${m}th day of the year
  const int currentYearlyIndex = ui->mYearlyCombo->currentIndex();
  ui->mYearlyCombo->clear();
  const QString longMonthName = calSys->monthName( date );
  item = subsOrdinal( ki18nc( "example: the 5th of June", "the %1 of %2" ), date.day() ).
         subs( longMonthName ).toString();
  ui->mYearlyCombo->addItem( item );

  item = subsOrdinal(
    ki18nc( "example: the 3rd to last day of June", "the %1 to last day of %2" ),
    date.daysInMonth() - date.day() ).subs( longMonthName ).toString();
  ui->mYearlyCombo->addItem( item );

  item = subsOrdinal(
    ki18nc( "example: the 4th Wednesday of June", "the %1 %2 of %3" ), monthWeekFromStart() ).
         subs( calSys->weekDayName( date.dayOfWeek(), KCalendarSystem::LongDayName ) ).
         subs( longMonthName ).toString();
  ui->mYearlyCombo->addItem( item );

  if ( monthWeekFromEnd() == 1 ) {
    item = ki18nc( "example: the last Wednesday of June", "the last %1 of %2" ).
           subs( calSys->weekDayName( date.dayOfWeek(), KCalendarSystem::LongDayName ) ).
           subs( longMonthName ).toString();
  } else {
    item = subsOrdinal(
      ki18nc( "example: the 4th to last Wednesday of June", "the %1 to last %2 of %3 " ),
      monthWeekFromEnd() ).
           subs( calSys->weekDayName( date.dayOfWeek(), KCalendarSystem::LongDayName ) ).
           subs( longMonthName ).toString();
  }
  ui->mYearlyCombo->addItem( item );

  item = subsOrdinal(
    ki18nc( "example: the 15th day of the year", "the %1 day of the year" ),
    date.dayOfYear() ).toString();
  ui->mYearlyCombo->addItem( item );
  ui->mYearlyCombo->setCurrentIndex( currentYearlyIndex == -1 ? 0 : currentYearlyIndex );
  */
}

