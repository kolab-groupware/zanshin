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


#include "editorview.h"
#include "recurrencewidget.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QUrl>
#include <KRun>
#include <QDebug>
#include "kdateedit.h"
#include "addressline/addresseelineedit.h"
#include "presentation/metatypes.h"

#include "domain/artifact.h"
#include "domain/task.h"
#include "domain/relation.h"


using namespace Widgets;

EditorView::EditorView(QWidget *parent)
    : QWidget(parent),
      m_model(0),
      m_delegateLabel(new QLabel(this)),
      m_textEdit(new QPlainTextEdit(this)),
      m_taskGroup(new QWidget(this)),
      m_startDateEdit(new KPIM::KDateEdit(m_taskGroup)),
      m_dueDateEdit(new KPIM::KDateEdit(m_taskGroup)),
      m_startTodayButton(new QPushButton(tr("Start today"), m_taskGroup)),
      m_delegateEdit(0),
      m_statusComboBox(new QComboBox(m_taskGroup)),
      m_progressEdit(new QSpinBox(m_taskGroup)),
      m_relationsLayout(new QVBoxLayout)
{
    // To avoid having unit tests talking to akonadi
    // while we don't need the completion for them
    if (qgetenv("ZANSHIN_UNIT_TEST_RUN").isEmpty())
        m_delegateEdit = new KPIM::AddresseeLineEdit(this);
    else
        m_delegateEdit = new KLineEdit(this);

    m_delegateLabel->setObjectName("delegateLabel");
    m_delegateEdit->setObjectName("delegateEdit");
    m_textEdit->setObjectName("textEdit");
    m_startDateEdit->setObjectName("startDateEdit");
    m_dueDateEdit->setObjectName("dueDateEdit");
    m_startTodayButton->setObjectName("startTodayButton");
    m_statusComboBox->setObjectName("statusComboBox");
    m_progressEdit->setObjectName("progressEdit");

    m_startDateEdit->setMinimumContentsLength(10);
    m_dueDateEdit->setMinimumContentsLength(10);

    m_progressEdit->setRange(0, 100);

    m_statusComboBox->addItem(tr("None"), Domain::Task::None);
    m_statusComboBox->addItem(tr("Needs action"), Domain::Task::NeedsAction);
    m_statusComboBox->addItem(tr("In process"), Domain::Task::InProcess);
    m_statusComboBox->addItem(tr("Completed"), Domain::Task::Complete);
    m_statusComboBox->addItem(tr("Cancelled"), Domain::Task::Cancelled);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(m_delegateLabel);
    layout->addWidget(m_textEdit);
    layout->addLayout(m_relationsLayout);
    layout->addWidget(m_taskGroup);
    setLayout(layout);

    QVBoxLayout *vbox = new QVBoxLayout;
    m_recurrenceTask = new QLabel(tr("This is one occurence of a recurrenting task"));
    m_recurrenceTask->setVisible(false);
    vbox->addWidget(m_recurrenceTask);
    auto delegateHBox = new QHBoxLayout;
    delegateHBox->addWidget(new QLabel(tr("Delegate to"), m_taskGroup));
    delegateHBox->addWidget(m_delegateEdit);
    vbox->addLayout(delegateHBox);
    QHBoxLayout *datesHBox = new QHBoxLayout;
    datesHBox->addWidget(new QLabel(tr("Start date"), m_taskGroup));
    datesHBox->addWidget(m_startDateEdit, 1);
    datesHBox->addWidget(new QLabel(tr("Due date"), m_taskGroup));
    datesHBox->addWidget(m_dueDateEdit, 1);
    vbox->addLayout(datesHBox);
    QHBoxLayout *bottomHBox = new QHBoxLayout;
    bottomHBox->addWidget(m_startTodayButton);
    bottomHBox->addStretch();
    vbox->addLayout(bottomHBox);
    auto progressHBox = new QHBoxLayout;
    progressHBox->addWidget(new QLabel(tr("Progress"), m_taskGroup));
    progressHBox->addWidget(m_progressEdit, 1);
    vbox->addLayout(progressHBox);
    auto statusHBox = new QHBoxLayout;
    statusHBox->addWidget(new QLabel(tr("Status"), m_taskGroup));
    statusHBox->addWidget(m_statusComboBox, 1);
    vbox->addLayout(statusHBox);
    m_recurrenceWidget = new RecurrenceWidget;
    vbox->addWidget(m_recurrenceWidget);
    m_taskGroup->setLayout(vbox);

    // Make sure our minimum width is always the one with
    // the task group visible
    layout->activate();
    setMinimumWidth(minimumSizeHint().width());

    m_delegateLabel->setVisible(false);
    m_taskGroup->setVisible(false);

    connect(m_textEdit, SIGNAL(textChanged()), this, SLOT(onTextEditChanged()));
    connect(m_startDateEdit, SIGNAL(dateEntered(QDate)), this, SLOT(onStartEditEntered(QDate)));
    connect(m_dueDateEdit, SIGNAL(dateEntered(QDate)), this, SLOT(onDueEditEntered(QDate)));
    connect(m_startTodayButton, SIGNAL(clicked()), this, SLOT(onStartTodayClicked()));
    connect(m_delegateEdit, SIGNAL(returnPressed()), this, SLOT(onDelegateEntered()));
    connect(m_progressEdit, SIGNAL(valueChanged(int)), this, SLOT(onProgressChanged(int)));
    connect(m_statusComboBox, SIGNAL(activated(int)), this, SLOT(onStatusChanged(int)));

    setEnabled(false);
}

QObject *EditorView::model() const
{
    return m_model;
}

void EditorView::setModel(QObject *model)
{
    if (model == m_model)
        return;

    if (m_model) {
        disconnect(m_model, 0, this, 0);
        disconnect(this, 0, m_model, 0);
    }

    m_model = model;

    onArtifactChanged();
    onTextOrTitleChanged();
    onHasTaskPropertiesChanged();
    onStartDateChanged();
    onDueDateChanged();
    onDelegateTextChanged();
    onProgressChanged();
    onRecurrenceChanged();
    onRelationsChanged();
    onStatusChanged();

    connect(m_model, SIGNAL(artifactChanged(Domain::Artifact::Ptr)),
            this, SLOT(onArtifactChanged()));
    connect(m_model, SIGNAL(hasTaskPropertiesChanged(bool)),
            this, SLOT(onHasTaskPropertiesChanged()));
    connect(m_model, SIGNAL(titleChanged(QString)), this, SLOT(onTextOrTitleChanged()));
    connect(m_model, SIGNAL(textChanged(QString)), this, SLOT(onTextOrTitleChanged()));
    connect(m_model, SIGNAL(startDateChanged(QDateTime)), this, SLOT(onStartDateChanged()));
    connect(m_model, SIGNAL(startDateChanged(QDateTime)), m_recurrenceWidget, SLOT(setStartDate(QDateTime)));
    connect(m_model, SIGNAL(dueDateChanged(QDateTime)), this, SLOT(onDueDateChanged()));
    connect(m_model, SIGNAL(delegateTextChanged(QString)), this, SLOT(onDelegateTextChanged()));
    connect(m_model, SIGNAL(progressChanged(int)), this, SLOT(onProgressChanged()));
    connect(m_model, SIGNAL(statusChanged(int)), this, SLOT(onStatusChanged()));
    connect(m_model, SIGNAL(recurrenceChanged(Domain::Recurrence::Ptr)), this, SLOT(onRecurrenceChanged()));
    connect(m_model, SIGNAL(relationsChanged(QList<Domain::Relation::Ptr>)), this, SLOT(onRelationsChanged()));

    connect(this, SIGNAL(titleChanged(QString)), m_model, SLOT(setTitle(QString)));
    connect(this, SIGNAL(textChanged(QString)), m_model, SLOT(setText(QString)));
    connect(this, SIGNAL(startDateChanged(QDateTime)), m_model, SLOT(setStartDate(QDateTime)));
    connect(this, SIGNAL(dueDateChanged(QDateTime)), m_model, SLOT(setDueDate(QDateTime)));
    connect(this, SIGNAL(delegateChanged(QString, QString)), m_model, SLOT(setDelegate(QString, QString)));
    connect(this, SIGNAL(progressChanged(int)), m_model, SLOT(setProgress(int)));
    connect(this, SIGNAL(statusChanged(int)), m_model, SLOT(setStatus(int)));

    connect(m_recurrenceWidget, SIGNAL(frequencyChanged(Domain::Recurrence::Frequency,int)),
        m_model, SLOT(setFrequency(Domain::Recurrence::Frequency, int)));
    connect(m_recurrenceWidget, SIGNAL(endChanged(QDateTime)),
        m_model, SLOT(setRepeatEnd(QDateTime)));
    connect(m_recurrenceWidget, SIGNAL(endChanged(int)),
        m_model, SLOT(setRepeatEnd(int)));
    connect(m_recurrenceWidget, SIGNAL(noEnd()),
        m_model, SLOT(setNoRepeat()));
    connect(m_recurrenceWidget, SIGNAL(exceptionDatesChanged(QList<QDateTime>)),
        m_model, SLOT(setExceptionDates(QList<QDateTime>)));
    connect(m_recurrenceWidget, SIGNAL(byDayChanged(QList<Domain::Recurrence::Weekday>)),
        m_model, SLOT(setByDay(QList<Domain::Recurrence::Weekday>)));
}

void EditorView::onArtifactChanged()
{
    auto artifact = m_model->property("artifact").value<Domain::Artifact::Ptr>();
    setEnabled(artifact);
}

void EditorView::onHasTaskPropertiesChanged()
{
    m_taskGroup->setVisible(m_model->property("hasTaskProperties").toBool());
}

void EditorView::onTextOrTitleChanged()
{
    const QString text = m_model->property("title").toString()
                       + "\n"
                       + m_model->property("text").toString();

    if (text != m_textEdit->toPlainText())
        m_textEdit->setPlainText(text);
}

void EditorView::onStartDateChanged()
{
    m_startDateEdit->setDate(m_model->property("startDate").toDateTime().date());
}

void EditorView::onDueDateChanged()
{
    m_dueDateEdit->setDate(m_model->property("dueDate").toDateTime().date());
}

void EditorView::onProgressChanged()
{
    m_progressEdit->setValue(m_model->property("progress").toInt());
}

void EditorView::onStatusChanged()
{
    for (int i = 0; i < m_statusComboBox->count(); i++) {
        if (m_statusComboBox->itemData(i).toInt() == m_model->property("status").toInt()) {
            m_statusComboBox->setCurrentIndex(i);
            return;
        }
    }
    m_statusComboBox->setCurrentIndex(0);
}

void EditorView::onDelegateTextChanged()
{
    const auto delegateText = m_model->property("delegateText").toString();
    const auto labelText = delegateText.isEmpty() ? QString()
                         : tr("Delegated to: <b>%1</b>").arg(delegateText);

    m_delegateLabel->setVisible(!labelText.isEmpty());
    m_delegateLabel->setText(labelText);
    m_delegateEdit->clear();
}

void EditorView::onRelationsChanged()
{
    const auto relations = m_model->property("relations").value<QList<Domain::Relation::Ptr> >();
    for (auto widget : m_relationWidgets) {
        m_relationsLayout->removeWidget(widget);
        delete widget;
    }
    m_relationWidgets.clear();

    for (auto relation : relations) {
        auto widget = new QWidget(this);
        auto layout = new QHBoxLayout(widget);
        widget->setLayout(layout);

        auto labelText = QString("<a href=\"%1\">%2</a>").arg(relation->url().toString()).arg(relation->name());
        auto label = new QLabel(widget);
        label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
        label->setTextFormat(Qt::RichText);
        label->setText(labelText);
        connect(label, SIGNAL(linkActivated(QString)), this, SLOT(onLinkActivated(QString)));
        layout->addWidget(label);

        auto button = new QPushButton(widget);
        button->setProperty("relation", QVariant::fromValue(relation));
        button->setIcon(QIcon::fromTheme("list-remove"));
        connect(button, SIGNAL(clicked()), this, SLOT(onRemoveRelationClicked()));
        layout->addWidget(button);
        layout->addStretch();

        m_relationsLayout->addWidget(widget);
        m_relationWidgets << widget;
    }
}

void EditorView::onRecurrenceChanged()
{
    const auto recurrence = m_model->property("recurrence").value<Domain::Recurrence::Ptr>();

    m_recurrenceWidget->blockSignals(true);
    if (recurrence) {
        m_recurrenceWidget->setRecurrenceType(recurrence->frequency());
        m_recurrenceWidget->setRecurrenceIntervall(recurrence->interval());
        m_recurrenceWidget->setExceptionDateTimes(recurrence->exceptionDates());

        m_recurrenceWidget->setByDay(recurrence->byday());

        if (recurrence->end().isValid()) {
            m_recurrenceWidget->setEnd(1);
            m_recurrenceWidget->setEnd(recurrence->end());
        } else if (recurrence->count() > 0) {
            m_recurrenceWidget->setEnd(QDateTime::currentDateTime());
            m_recurrenceWidget->setEnd(recurrence->count());
        } else {
            m_recurrenceWidget->setEnd(1);
            m_recurrenceWidget->setEnd(QDateTime::currentDateTime());
            m_recurrenceWidget->setNoEnd();
        }
    } else {
        m_recurrenceWidget->clear();
    }
    m_recurrenceWidget->blockSignals(false);

    if (recurrence && !m_statusComboBox->itemData(5).isValid()) {
        m_statusComboBox->addItem(tr("All ocurrences completed"), Domain::Task::FullComplete);
        onStatusChanged();
        m_recurrenceTask->setVisible(true);
    } else if (!recurrence && m_statusComboBox->itemData(5).isValid()) {
        m_statusComboBox->removeItem(5);
        m_recurrenceTask->setVisible(false);
    }
}


void EditorView::onRemoveRelationClicked()
{
    auto relation = sender()->property("relation").value<Domain::Relation::Ptr>();
    QMetaObject::invokeMethod(m_model, "removeRelation",
                                Q_ARG(Domain::Relation::Ptr, relation));
}

void EditorView::onLinkActivated(const QString &link)
{
    new KRun(link, this);
}

void EditorView::onTextEditChanged()
{
    const QString plainText = m_textEdit->toPlainText();
    const int index = plainText.indexOf('\n');
    const QString title = plainText.left(index);
    const QString text = plainText.mid(index + 1);
    emit titleChanged(title);
    emit textChanged(text);
}

void EditorView::onStartEditEntered(const QDate &start)
{
    emit startDateChanged(QDateTime(start));
}

void EditorView::onDueEditEntered(const QDate &due)
{
    emit dueDateChanged(QDateTime(due));
}

void EditorView::onStartTodayClicked()
{
    QDate today(QDate::currentDate());
    m_startDateEdit->setDate(today);
    emit startDateChanged(QDateTime(today));
}

void EditorView::onDelegateEntered()
{
    const auto input = m_delegateEdit->text();
    auto name = QString();
    auto email = QString();
    auto gotMatch = false;

    QRegExp fullRx("\\s*(.*) <([\\w\\.]+@[\\w\\.]+)>\\s*");
    QRegExp emailOnlyRx("\\s*<?([\\w\\.]+@[\\w\\.]+)>?\\s*");

    if (input.contains(fullRx)) {
        name = fullRx.cap(1);
        email = fullRx.cap(2);
        gotMatch = true;
    } else if (input.contains(emailOnlyRx)) {
        email = emailOnlyRx.cap(1);
        gotMatch = true;
    }

    if (gotMatch) {
        QMetaObject::invokeMethod(m_model, "delegate",
                                  Q_ARG(QString, name),
                                  Q_ARG(QString, email));
    }
    emit delegateChanged(name, email);
}

void EditorView::onProgressChanged(int progress)
{
    emit progressChanged(progress);
}

void EditorView::onStatusChanged(int index)
{
    emit statusChanged(m_statusComboBox->itemData(index).toInt());
}
