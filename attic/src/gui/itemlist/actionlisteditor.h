/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>

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

#ifndef ZANSHIN_ACTIONLISTEDITOR_H
#define ZANSHIN_ACTIONLISTEDITOR_H

#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

#include "globaldefs.h"
#include <KXMLGUIClient>

class CollectionsFilterProxyModel;
class ItemEditor;
class ItemSelectorProxy;
class ActionListEditorPage;
class KAction;
class KActionCollection;
class KConfigGroup;
class KLineEdit;
class QAbstractItemModel;
class QComboBox;
class QItemSelectionModel;
class QStackedWidget;
class ModelStack;

class ActionListEditor : public QWidget
{
    Q_OBJECT

public:
    ActionListEditor(ModelStack *models,
                     KActionCollection *ac, QWidget *parent, KXMLGUIClient *client, ItemEditor *itemviewer);

    void setMode(Zanshin::ApplicationMode mode);

    void saveColumnsState(KConfigGroup &config) const;
    void restoreColumnsState(const KConfigGroup &config);

protected:
    virtual bool eventFilter(QObject *watched, QEvent *event);
/*signals:
    void currentChanged(const Akonadi::Item &);*/

private slots:
    void updateActions();
    void onRemoveAction();
    void onMoveAction();
    void onPromoteAction();
    void onDissociateAction();
    void focusActionEdit();
    void clearActionEdit();
    void onSideBarSelectionChanged(const QModelIndex &index);


private:
    void createPage(QAbstractItemModel *model, ModelStack *models, Zanshin::ApplicationMode, KXMLGUIClient *guiClient);
    void setupActions(KActionCollection *ac);

    QAbstractItemModel *currentSidebarModel(Zanshin::ApplicationMode mode) const;
    QItemSelectionModel *currentSelection(Zanshin::ApplicationMode mode) const;
    ActionListEditorPage *currentPage() const;
    ActionListEditorPage *page(int idx) const;

    QStackedWidget *m_stack;

    KAction *m_add;
    KAction *m_cancelAdd;
    KAction *m_remove;
    KAction *m_move;
    KAction *m_promote;
    KAction *m_dissociate;

    ModelStack *m_models;
    ItemSelectorProxy *m_selectorProxy;
    
};

#endif

