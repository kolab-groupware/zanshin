/* This file is part of Zanshin Todo.

   Copyright 2008-2009 Kevin Ottens <ervin@kde.org>

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

#include "sidebar.h"

#include <KDE/Akonadi/AgentManager>
#include <KDE/Akonadi/AgentInstance>
#include <KDE/Akonadi/AgentType>
#include <KDE/Akonadi/EntityDisplayAttribute>
#include <KDE/Akonadi/EntityTreeView>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KIcon>
#include <KDE/KLocale>

#include <QtGui/QStackedWidget>
#include <QtGui/QToolBar>
#include <QtGui/QVBoxLayout>

#include "globaldefs.h"
#include "sidebarpage.h"
#include "core/modelstack.h"


SideBar::SideBar(ModelStack *models, KActionCollection *ac, QWidget *parent)
    : QWidget(parent)
{
    setupActions(ac);

    setLayout(new QVBoxLayout(this));
    m_stack = new QStackedWidget(this);
    layout()->addWidget(m_stack);
    layout()->setContentsMargins(0, 0, 0, 0);

    createPage(models->treeSideBarModel(), models->treeSelection());
    createPage(models->contextsSideBarModel(), models->contextsSelection());
    createPage(models->knowledgeSideBarModel(), models->knowledgeSelection());
    
    setupToolBar();
}

void SideBar::createPage(QAbstractItemModel *model, QItemSelectionModel *selectionModel)
{
    QAction *separator = new QAction(this);
    separator->setSeparator(true);

    QList<QAction*> contextActions;
    contextActions << m_add
                   << m_remove
                   << separator
                   << m_rename;

    SideBarPage *page = new SideBarPage(model, selectionModel, contextActions, m_stack);

    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this, SLOT(updateActions(QModelIndex)));

    m_stack->addWidget(page);
}

void SideBar::setupToolBar()
{
    QHBoxLayout *toolbarLayout = new QHBoxLayout;
    toolbarLayout->setAlignment(Qt::AlignRight);
    static_cast<QBoxLayout*>(layout())->addLayout(toolbarLayout);

    QToolBar *projectBar = new QToolBar(this);
    projectBar->setIconSize(QSize(16, 16));
    projectBar->addAction(m_synchronize);
    projectBar->addAction(m_add);
    projectBar->addAction(m_remove);

    toolbarLayout->addWidget(projectBar);
}

void SideBar::setupActions(KActionCollection *ac)
{
    m_add = ac->addAction("sidebar_new", this, SLOT(onAddItem()));
    m_add->setText(i18n("New"));
    m_add->setIcon(KIcon("list-add"));

    m_remove = ac->addAction("sidebar_remove", this, SLOT(onRemoveItem()));
    m_remove->setText(i18n("Remove"));
    m_remove->setIcon(KIcon("list-remove"));

    m_rename = ac->addAction("sidebar_rename", this, SLOT(onRenameItem()));
    m_rename->setText(i18n("Rename"));
    m_rename->setIcon(KIcon("edit-rename"));

    m_previous = ac->addAction("sidebar_go_previous", this, SLOT(onPreviousItem()));
    m_previous->setText(i18n("Previous"));
    m_previous->setIcon(KIcon("go-previous"));
    m_previous->setShortcut(Qt::ALT | Qt::Key_Up);

    m_next = ac->addAction("sidebar_go_next", this, SLOT(onNextItem()));
    m_next->setText(i18n("Next"));
    m_next->setIcon(KIcon("go-next"));
    m_next->setShortcut(Qt::ALT | Qt::Key_Down);

    m_synchronize = ac->addAction("sidebar_synchronize", this, SLOT(onSynchronize()));
    m_synchronize->setText(i18n("Synchronize"));
    m_synchronize->setIcon(KIcon("view-refresh"));
    m_synchronize->setShortcut(Qt::Key_F5);
}

void SideBar::setMode(Zanshin::ApplicationMode mode)
{
    switch (mode) {
    case Zanshin::ProjectMode:
        m_stack->setCurrentIndex(0);
        m_add->setText(i18n("New Project"));
        m_remove->setText(i18n("Remove Project"));
        m_rename->setText(i18n("Rename Project"));
        m_previous->setText(i18n("Previous Project"));
        m_next->setText(i18n("Next Project"));
        break;

    case Zanshin::ContextsMode:
        m_stack->setCurrentIndex(1);
        m_add->setText(i18n("New Context"));
        m_remove->setText(i18n("Remove Context"));
        m_rename->setText(i18n("Rename Context"));
        m_previous->setText(i18n("Previous Context"));
        m_next->setText(i18n("Next Context"));
        break;
    case Zanshin::KnowledgeMode:
        m_stack->setCurrentIndex(2);
        m_add->setText(i18n("New Topic"));
        m_remove->setText(i18n("Remove Topic"));
        m_rename->setText(i18n("Rename Topic"));
        m_previous->setText(i18n("Previous Topic"));
        m_next->setText(i18n("Next Topic"));
        break;
    }

    updateActions(currentPage()->selectionModel()->currentIndex());
}

static Akonadi::Collection getCollection(const QModelIndex &index)
{
    const Zanshin::ItemType type = (Zanshin::ItemType) index.data(Zanshin::ItemTypeRole).toInt();

    Akonadi::Collection col;
    if ( type==Zanshin::Collection ) {
        col = index.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    } else {
        // We use ParentCollectionRole instead of Akonadi::Item::parentCollection() because the
        // information about the rights is not valid on retrieved items.
        col = index.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
    }
    return col;
}

void SideBar::updateActions(const QModelIndex &index)
{
    const Zanshin::ItemType type = (Zanshin::ItemType) index.data(Zanshin::ItemTypeRole).toInt();
    const Akonadi::Collection col = getCollection(index);
    m_add->setEnabled((col.rights() & Akonadi::Collection::CanCreateItem)
                  && ( type == Zanshin::Collection
                    || type == Zanshin::ProjectTodo
                    || type == Zanshin::ContextRoot
                    || type == Zanshin::Context
                    || type == Zanshin::TopicRoot 
                    || type == Zanshin::Topic ));

    m_remove->setEnabled((col.rights() & Akonadi::Collection::CanDeleteItem)
                      && (type == Zanshin::ProjectTodo
                       || type == Zanshin::Context 
                       || type == Zanshin::Topic ));

    m_rename->setEnabled((col.rights() & Akonadi::Collection::CanChangeItem)
                      && (type == Zanshin::ProjectTodo
                       || type == Zanshin::Context 
                       || type == Zanshin::Topic ));

    QString name;

    if ( !col.isValid() ) {
        name = index.data().toString();
    } else {
        if ( col.hasAttribute<Akonadi::EntityDisplayAttribute>() &&
             !col.attribute<Akonadi::EntityDisplayAttribute>()->displayName().isEmpty() ) {
            name = col.attribute<Akonadi::EntityDisplayAttribute>()->displayName();
        } else {
            name = col.name();
        }
    }

    m_synchronize->setData(QVariant::fromValue(col));
    m_synchronize->setText(i18n("Synchronize \"%1\"", name));
}

void SideBar::onAddItem()
{
    currentPage()->addNewItem();
}

void SideBar::onRemoveItem()
{
    currentPage()->removeCurrentItem();
}

void SideBar::onRenameItem()
{
    currentPage()->renameCurrentItem();
}

void SideBar::onPreviousItem()
{
    currentPage()->selectPreviousItem();
}

void SideBar::onNextItem()
{
    currentPage()->selectNextItem();
}

void SideBar::onSynchronize()
{
    KAction *action = static_cast<KAction*>(sender());
    Akonadi::Collection col = action->data().value<Akonadi::Collection>();

    if (col.isValid()) {
        Akonadi::AgentManager::self()->synchronizeCollection(col);
    } else {
        Akonadi::AgentInstance::List agents = Akonadi::AgentManager::self()->instances();
        while (!agents.isEmpty()) {
            Akonadi::AgentInstance agent = agents.takeFirst();

            if (agent.type().mimeTypes().contains("application/x-vnd.akonadi.calendar.todo")) {
                agent.synchronize();
            }
        }
    }
}

SideBarPage *SideBar::currentPage() const
{
    return static_cast<SideBarPage*>(m_stack->currentWidget());
}

SideBarPage *SideBar::page(int idx) const
{
    return static_cast<SideBarPage*>(m_stack->widget(idx));
}

