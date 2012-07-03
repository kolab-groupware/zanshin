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

#include "actionlisteditorpage.h"

#include <KDE/Akonadi/ItemCreateJob>

#include <KDE/KConfigGroup>
#include <kdescendantsproxymodel.h>
#include <kmodelindexproxymapper.h>

#include <QtCore/QTimer>
#include <QtGui/QHeaderView>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QVBoxLayout>

#include "modelstack.h"
#include "actionlistcombobox.h"
#include "actionlistdelegate.h"
#include "categorymanager.h"
#include "globaldefs.h"
#include "todotreeview.h"
#include "todohelpers.h"
#include <KXMLGUIClient>
#include <filterproxymodel.h>
#include <note.h>
#include <QComboBox>
#include <KPassivePopup>
#include <KLineEdit>
#include <QToolBar>
#include <Akonadi/ItemDeleteJob>
#include <KGlobal>
#include <KLocalizedString>
#include <searchbar.h>

static const char *_z_defaultColumnStateCache = "AAAA/wAAAAAAAAABAAAAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAvAAAAAFAQEAAQAAAAAAAAAAAAAAAGT/////AAAAgQAAAAAAAAAFAAABNgAAAAEAAAAAAAAAlAAAAAEAAAAAAAAAjQAAAAEAAAAAAAAAcgAAAAEAAAAAAAAAJwAAAAEAAAAA";

class GroupLabellingProxyModel : public QSortFilterProxyModel
{
public:
    GroupLabellingProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (role!=Qt::DisplayRole || index.column()!=0) {
            return QSortFilterProxyModel::data(index, role);
        } else {
            int type = index.data(Zanshin::ItemTypeRole).toInt();

            if (type!=Zanshin::ProjectTodo
             && type!=Zanshin::Category) {
                return QSortFilterProxyModel::data(index, role);

            } else {
                QString display = QSortFilterProxyModel::data(index, role).toString();

                QModelIndex currentIndex = mapToSource(index.parent());
                type = currentIndex.data(Zanshin::ItemTypeRole).toInt();

                while (type==Zanshin::ProjectTodo
                    || type==Zanshin::Category) {
                    display = currentIndex.data().toString() + ": " + display;

                    currentIndex = currentIndex.parent();
                    type = currentIndex.data(Zanshin::ItemTypeRole).toInt();
                }

                return display;
            }
        }
    }
};

class GroupSortingProxyModel : public QSortFilterProxyModel
{
public:
    GroupSortingProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const
    {
        int leftType = left.data(Zanshin::ItemTypeRole).toInt();
        int rightType = right.data(Zanshin::ItemTypeRole).toInt();

        return leftType==Zanshin::Inbox
            || (leftType==Zanshin::CategoryRoot && rightType!=Zanshin::Inbox)
            || (leftType==Zanshin::Collection && rightType!=Zanshin::Inbox)
            || (leftType==Zanshin::StandardTodo && rightType!=Zanshin::StandardTodo)
            || (leftType==Zanshin::ProjectTodo && rightType==Zanshin::Collection)
            || (leftType == rightType && QSortFilterProxyModel::lessThan(left, right));
    }
};

class TypeFilterProxyModel : public QSortFilterProxyModel
{
public:
    TypeFilterProxyModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
    {
        setDynamicSortFilter(true);
        setFilterCaseSensitivity(Qt::CaseInsensitive);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex sourceChild = sourceModel()->index(sourceRow, 0, sourceParent);
        int type = sourceChild.data(Zanshin::ItemTypeRole).toInt();

        QSize sizeHint = sourceChild.data(Qt::SizeHintRole).toSize();

        return type!=Zanshin::Collection
            && type!=Zanshin::CategoryRoot
            && type!=Zanshin::TopicRoot
            && !sizeHint.isNull(); // SelectionProxyModel uses the null size for items we shouldn't display
    }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
    {
        if (sourceModel()) {
            sourceModel()->sort(column, order);
        }
    }

private:
    GroupSortingProxyModel *m_sorting;
};


class ActionListEditorView : public TodoTreeView
{
public:
    ActionListEditorView(QWidget *parent = 0)
        : TodoTreeView(parent) { }

protected:
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
    {
        QModelIndex index = currentIndex();

        if (index.isValid() && modifiers==Qt::NoModifier) {
            QModelIndex newIndex;
            int newColumn = index.column();

            switch (cursorAction) {
            case MoveLeft:
                do {
                    newColumn--;
                } while (isColumnHidden(newColumn)
                      && newColumn>=0);
                break;

            case MoveRight:
                do {
                    newColumn++;
                } while (isColumnHidden(newColumn)
                      && newColumn<header()->count());
                break;

            default:
                return Akonadi::EntityTreeView::moveCursor(cursorAction, modifiers);
            }

            newIndex = index.sibling(index.row(), newColumn);

            if (newIndex.isValid()) {
                return newIndex;
            }
        }

        return Akonadi::EntityTreeView::moveCursor(cursorAction, modifiers);
    }
};

class ActionListEditorModel : public KDescendantsProxyModel
{
public:
    ActionListEditorModel(QObject *parent = 0)
        : KDescendantsProxyModel(parent)
    {
    }

    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
    {
        if (!sourceModel()) {
            return QAbstractProxyModel::dropMimeData(data, action, row, column, parent);
        }
        QModelIndex sourceParent = mapToSource(parent);
        return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
    }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
    {
        if (sourceModel()) {
            sourceModel()->sort(column, order);
        }
    }
};

class CollectionsFilterProxyModel : public QSortFilterProxyModel
{
public:
    CollectionsFilterProxyModel(const QString &mimetype, QObject *parent = 0)
        : QSortFilterProxyModel(parent),
        m_mimetype(mimetype)
    {
        setDynamicSortFilter(true);
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex sourceChild = sourceModel()->index(sourceRow, 0, sourceParent);
        Akonadi::Collection col = sourceChild.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

        return col.isValid()
            && col.contentMimeTypes().contains(m_mimetype)
            && (col.rights() & (Akonadi::Collection::CanChangeItem|Akonadi::Collection::CanCreateItem));
    }

    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder)
    {
        if (sourceModel()) {
            sourceModel()->sort(column, order);
        }
    }
private:
    const QString m_mimetype;
};


Configuration::Configuration() : QObject(){};


void Configuration::setDefaultTodoCollection(const Akonadi::Collection &collection) {
    KConfigGroup config(KGlobal::config(), "General");
    config.writeEntry("defaultCollection", QString::number(collection.id()));
    config.sync();
    emit defaultTodoCollectionChanged(collection);
}

Akonadi::Collection Configuration::defaultTodoCollection() {
    KConfigGroup config(KGlobal::config(), "General");
    Akonadi::Collection::Id id = config.readEntry("defaultCollection", -1);
    return Akonadi::Collection(id);
}

void Configuration::setDefaultNoteCollection(const Akonadi::Collection &collection) {
    KConfigGroup config(KGlobal::config(), "General");
    config.writeEntry("defaultNoteCollection", QString::number(collection.id()));
    config.sync();
    emit defaultNoteCollectionChanged(collection);
}

Akonadi::Collection Configuration::defaultNoteCollection() {
    KConfigGroup config(KGlobal::config(), "General");
    Akonadi::Collection::Id id = config.readEntry("defaultNoteCollection", -1);
    return Akonadi::Collection(id);
}

ActionListEditorPage::ActionListEditorPage(QAbstractItemModel *model,
                                           ModelStack *models,
                                           Zanshin::ApplicationMode mode,
                                           const QList<QAction*> &contextActions,
                                           const QList<QAction*> &toolbarActions,
                                           QWidget *parent, KXMLGUIClient *client)
    : QWidget(parent), 
    m_mode(mode),
    m_defaultCollectionId(-1)
{
    setLayout(new QVBoxLayout(this));
    layout()->setContentsMargins(0, 0, 0, 0);

    m_treeView = new ActionListEditorView(this);
    
    FilterProxyModel *notefilter = new FilterProxyModel(this);
    notefilter->setSourceModel(model);
    
    SearchBar *searchBar = new SearchBar(notefilter, this);
    layout()->addWidget(searchBar);

    GroupLabellingProxyModel *labelling = new GroupLabellingProxyModel(this);
    labelling->setSourceModel(notefilter);

    GroupSortingProxyModel *sorting = new GroupSortingProxyModel(this);
    sorting->setSourceModel(labelling);

    ActionListEditorModel *descendants = new ActionListEditorModel(this);
    descendants->setSourceModel(sorting);

    TypeFilterProxyModel *filter = new TypeFilterProxyModel(this);
    filter->setSourceModel(descendants);

    m_treeView->setModel(filter);
    m_treeView->setItemDelegate(new ActionListDelegate(models, m_treeView));

    m_treeView->header()->setSortIndicatorShown(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(0, Qt::AscendingOrder);

    m_treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_treeView->setItemsExpandable(false);
    m_treeView->setRootIsDecorated(false);
    m_treeView->setEditTriggers(m_treeView->editTriggers() | QAbstractItemView::DoubleClicked);

    connect(m_treeView->model(), SIGNAL(modelReset()),
            m_treeView, SLOT(expandAll()));
    connect(m_treeView->model(), SIGNAL(layoutChanged()),
            m_treeView, SLOT(expandAll()));
    connect(m_treeView->model(), SIGNAL(rowsInserted(QModelIndex,int,int)),
            m_treeView, SLOT(expandAll()));

    layout()->addWidget(m_treeView);

    QTimer::singleShot(0, this, SLOT(onAutoHideColumns()));

    connect(m_treeView->header(), SIGNAL(sectionResized(int,int,int)),
            this, SLOT(onColumnsGeometryChanged()));

    m_treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    m_treeView->addActions(contextActions);
    
    
    QWidget *bottomBar = new QWidget(this);
    layout()->addWidget(bottomBar);
    bottomBar->setLayout(new QHBoxLayout(bottomBar));
    bottomBar->layout()->setContentsMargins(0, 0, 0, 0);

    m_addActionEdit = new KLineEdit(bottomBar);
    m_addActionEdit->installEventFilter(this);
    bottomBar->layout()->addWidget(m_addActionEdit);
    m_addActionEdit->setClickMessage(i18n("Type and press enter to add an action"));
    m_addActionEdit->setClearButtonShown(true);
    connect(m_addActionEdit, SIGNAL(returnPressed()),
            this, SLOT(onAddActionRequested()));
    
    m_comboBox = new ActionListComboBox(bottomBar);
    m_comboBox->view()->setTextElideMode(Qt::ElideLeft);
    m_comboBox->setMinimumContentsLength(20);
    m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);

    connect(m_comboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onComboBoxChanged()));

    if (mode == Zanshin::KnowledgeMode) {
        KDescendantsProxyModel *descendantProxyModel = new KDescendantsProxyModel(m_comboBox);
        descendantProxyModel->setSourceModel(models->knowledgeCollectionsModel());
        descendantProxyModel->setDisplayAncestorData(true);
        m_todoColsModel = new CollectionsFilterProxyModel(AbstractPimItem::mimeType(AbstractPimItem::Note), m_comboBox);
        m_todoColsModel->setSourceModel(descendantProxyModel);
        m_defaultCollectionId = Configuration::instance().defaultNoteCollection().id();
    } else {
        KDescendantsProxyModel *descendantProxyModel = new KDescendantsProxyModel(m_comboBox);
        descendantProxyModel->setSourceModel(models->collectionsModel());
        descendantProxyModel->setDisplayAncestorData(true);
        m_todoColsModel = new CollectionsFilterProxyModel(AbstractPimItem::mimeType(AbstractPimItem::Todo), m_comboBox);
        m_todoColsModel->setSourceModel(descendantProxyModel);
        m_defaultCollectionId = Configuration::instance().defaultTodoCollection().id();
    }
    kDebug() << AbstractPimItem::mimeType(AbstractPimItem::Note);
    if (m_defaultCollectionId > 0) {
        if (!selectDefaultCollection(m_todoColsModel, QModelIndex(),
                                    0, m_todoColsModel->rowCount()-1, m_defaultCollectionId)) {
            connect(m_todoColsModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                    this, SLOT(onRowInsertedInComboBox(QModelIndex,int,int)));
        }
    }
    
    m_comboBox->setModel(m_todoColsModel);
    
    bottomBar->layout()->addWidget(m_comboBox);

    QToolBar *toolBar = new QToolBar(bottomBar);
    toolBar->setIconSize(QSize(16, 16));
    bottomBar->layout()->addWidget(toolBar);
    toolBar->addActions(toolbarActions);
    onComboBoxChanged();
    
    connect(&Configuration::instance(), SIGNAL(defaultNoteCollectionChanged(Akonadi::Collection)), this, SLOT(setDefaultNoteCollection(Akonadi::Collection)));
    connect(&Configuration::instance(), SIGNAL(defaultTodoCollectionChanged(Akonadi::Collection)), this, SLOT(setDefaultCollection(Akonadi::Collection)));

}

void ActionListEditorPage::setCollectionSelectorVisible(bool visible)
{
    m_comboBox->setVisible(visible);
}

void ActionListEditorPage::onComboBoxChanged()
{
    QModelIndex collectionIndex = m_comboBox->model()->index( m_comboBox->currentIndex(), 0 );
    Akonadi::Collection collection = collectionIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    if (m_mode == Zanshin::KnowledgeMode) { //TODO based on content not viewtype
        Configuration::instance().setDefaultNoteCollection(collection);
    } else {
        Configuration::instance().setDefaultTodoCollection(collection);
    }

}

void ActionListEditorPage::selectDefaultCollection(const Akonadi::Collection& collection)
{
    selectDefaultCollection(m_todoColsModel, QModelIndex(),
                                    0, m_todoColsModel->rowCount()-1, collection.id());
}

bool ActionListEditorPage::selectDefaultCollection(QAbstractItemModel *model, const QModelIndex &parent, int begin, int end, Akonadi::Collection::Id defaultCol)
{
    for (int i = begin; i <= end; i++) {
        QModelIndex collectionIndex = model->index(i, 0, parent);
        Akonadi::Collection collection = collectionIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        if (collection.id() == defaultCol) {
            m_comboBox->setCurrentIndex(i);
            m_defaultCollectionId = -1;
            return true;
        }
    }
    return false;
}

void ActionListEditorPage::onRowInsertedInComboBox(const QModelIndex &parent, int begin, int end)
{
    QAbstractItemModel *model = static_cast<QAbstractItemModel*>(sender());
    if (selectDefaultCollection(model, parent, begin, end, m_defaultCollectionId)) {
        disconnect(this, SLOT(onRowInsertedInComboBox(QModelIndex,int,int)));
    }
}

QItemSelectionModel *ActionListEditorPage::selectionModel() const
{
    return m_treeView->selectionModel();
}

Akonadi::EntityTreeView* ActionListEditorPage::treeView() const
{
    return m_treeView;
}

void ActionListEditorPage::saveColumnsState(KConfigGroup &config, const QString &key) const
{
    config.writeEntry(key+"/Normal", m_normalStateCache.toBase64());
    config.writeEntry(key+"/NoCollection", m_noCollectionStateCache.toBase64());
}

void ActionListEditorPage::restoreColumnsState(const KConfigGroup &config, const QString &key)
{
    if (config.hasKey(key+"/Normal")) {
        m_normalStateCache = QByteArray::fromBase64(config.readEntry(key+"/Normal", QByteArray()));
    } else {
        m_normalStateCache = QByteArray::fromBase64(_z_defaultColumnStateCache);
    }

    if (config.hasKey(key+"/NoCollection")) {
        m_noCollectionStateCache = QByteArray::fromBase64(config.readEntry(key+"/NoCollection", QByteArray()));
    }

    if (!m_treeView->isColumnHidden(PimItemModel::Collection)) {
        m_treeView->header()->restoreState(m_normalStateCache);
    } else {
        m_treeView->header()->restoreState(m_noCollectionStateCache);
    }
}


void ActionListEditorPage::addNewItem(const QString& summary)
{
    if (m_mode == Zanshin::KnowledgeMode) {
        addNewNote(summary);
    } else {
        addNewTodo(summary);
    }
}

void ActionListEditorPage::addNewNote(const QString& summary)
{
    if (!m_defaultNoteCollection.isValid()) {
        kWarning() << "no valid default collection";
        return;
    }
    kDebug() << "creating new note";
    Note note;
    note.setTitle(summary);
    Akonadi::ItemCreateJob *itemCreateJob = new Akonadi::ItemCreateJob(note.getItem(), m_defaultNoteCollection);
    //connect( itemCreateJob, SIGNAL(result(KJob*)), SLOT(itemCreateDone( KJob *)));
    //TODO set currently selected topic
}

void ActionListEditorPage::addNewTodo(const QString &summary)
{
    if (summary.isEmpty()) return;

    QModelIndex current = m_treeView->selectionModel()->currentIndex();

    if (!current.isValid()) {
        kWarning() << "Oops, nothing selected in the list!";
        return;
    }

    int type = current.data(Zanshin::ItemTypeRole).toInt();

    while (current.isValid() && type==Zanshin::StandardTodo) {
        current = current.sibling(current.row()-1, current.column());
        type = current.data(Zanshin::ItemTypeRole).toInt();
    }

    Akonadi::Collection collection;
    QString parentUid;
    QString category;

    switch (type) {
    case Zanshin::StandardTodo:
        kFatal() << "Can't possibly happen!";
        break;

    case Zanshin::ProjectTodo:
        parentUid = current.data(Zanshin::UidRole).toString();
        collection = current.data(Akonadi::EntityTreeModel::ParentCollectionRole).value<Akonadi::Collection>();
        break;

    case Zanshin::Collection:
        collection = current.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
        break;

    case Zanshin::Category:
        category = current.data(Zanshin::CategoryPathRole).toString(); //TODO
        // fallthrough
    case Zanshin::Inbox:
    case Zanshin::CategoryRoot:
        collection = m_defaultCollection;
        break;
    }

    TodoHelpers::addTodo(summary, parentUid, category, collection);
}

void ActionListEditorPage::removeCurrentItem()
{
    QModelIndex current = m_treeView->selectionModel()->currentIndex();
    removeItem(current);
}

void ActionListEditorPage::removeItem(const QModelIndex &current)
{
    int type = current.data(Zanshin::ItemTypeRole).toInt();

    if (!current.isValid()) {
        return;
    }
    if (type == Zanshin::StandardTodo) {
        TodoHelpers::removeProject(this, current);
    }
    if (type == AbstractPimItem::Note) {
        new Akonadi::ItemDeleteJob(current.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>());
    }
}

void ActionListEditorPage::dissociateTodo(const QModelIndex &current)
{
    int type = current.data(Zanshin::ItemTypeRole).toInt();

    if (!current.isValid()
     || type!=Zanshin::StandardTodo
     || m_mode==Zanshin::ProjectMode) {
        return;
    }

    for (int i=current.row(); i>=0; --i) {
        QModelIndex index = m_treeView->model()->index(i, 0);
        int type = index.data(Zanshin::ItemTypeRole).toInt();
        if (type==Zanshin::Category) {
            Id category = index.data(Zanshin::RelationIdRole).toInt();
            if (CategoryManager::contextInstance().dissociateFromCategory(current.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>(), category)) {
                break;
            }
        }
    }
}

Zanshin::ApplicationMode ActionListEditorPage::mode()
{
    return m_mode;
}

void ActionListEditorPage::onAutoHideColumns()
{
    switch (m_mode) {
    case Zanshin::ProjectMode:
        m_treeView->hideColumn(PimItemModel::Project);
        m_treeView->showColumn(PimItemModel::Contexts);
        m_treeView->hideColumn(PimItemModel::Status);
        break;
    case Zanshin::CategoriesMode:
        m_treeView->showColumn(PimItemModel::Project);
        m_treeView->hideColumn(PimItemModel::Contexts);
        m_treeView->hideColumn(PimItemModel::Status);
        break;
    case Zanshin::KnowledgeMode:
        m_treeView->hideColumn(PimItemModel::Status);
        m_treeView->hideColumn(PimItemModel::Collection);
        m_treeView->hideColumn(PimItemModel::Contexts);
        m_treeView->hideColumn(PimItemModel::Project);
        break;
    }
}

void ActionListEditorPage::setCollectionColumnHidden(bool hidden)
{
    QByteArray state = hidden ? m_noCollectionStateCache : m_normalStateCache;

    if (!state.isEmpty()) {
        m_treeView->header()->restoreState(state);
    } else {
        m_treeView->setColumnHidden(PimItemModel::Collection, hidden);
    }
}

void ActionListEditorPage::onColumnsGeometryChanged()
{
    if (!m_treeView->isColumnHidden(PimItemModel::Collection)) {
        m_normalStateCache = m_treeView->header()->saveState();
    } else {
        m_noCollectionStateCache = m_treeView->header()->saveState();
    }
}

void ActionListEditorPage::setDefaultCollection(const Akonadi::Collection &collection)
{
    //TODO select in combobox
    m_defaultCollection = collection;
    selectDefaultCollection(m_defaultCollection);
}

void ActionListEditorPage::setDefaultNoteCollection(const Akonadi::Collection& collection)
{
    m_defaultNoteCollection = collection;
    selectDefaultCollection(m_defaultCollection);
}

bool ActionListEditorPage::selectSiblingIndex(const QModelIndex &index)
{
    QModelIndex sibling = m_treeView->indexBelow(index);
    if (!sibling.isValid()) {
        sibling = m_treeView->indexAbove(index);
    }
    if (sibling.isValid()) {
        m_treeView->selectionModel()->setCurrentIndex(sibling, QItemSelectionModel::Select|QItemSelectionModel::Rows);
        return true;
    }
    return false;
}

void ActionListEditorPage::selectFirstIndex()
{
    QTimer::singleShot(0, this, SLOT(onSelectFirstIndex()));
}

void ActionListEditorPage::onSelectFirstIndex()
{
    // Clear selection to avoid multiple selections when a widget is in edit mode.
    m_treeView->selectionModel()->clearSelection();
    QModelIndex root = m_treeView->model()->index(0, 0);
    if (root.isValid()) {
        m_treeView->selectionModel()->setCurrentIndex(root, QItemSelectionModel::Select|QItemSelectionModel::Rows);
    }
}

void ActionListEditorPage::clearActionEdit()
{
    m_addActionEdit->clear();
}

void ActionListEditorPage::focusActionEdit()
{
    QPoint pos = m_addActionEdit->geometry().topLeft();
    pos = m_addActionEdit->parentWidget()->mapToGlobal(pos);

    KPassivePopup *popup = KPassivePopup::message(i18n("Type and press enter to add an action"), m_addActionEdit);
    popup->move(pos-QPoint(0, popup->height()));
    m_addActionEdit->setFocus();
}

void ActionListEditorPage::setActionEditEnabled(bool enabled)
{
    m_addActionEdit->setEnabled(enabled);
}


void ActionListEditorPage::onAddActionRequested()
{
    QString summary = m_addActionEdit->text().trimmed();
    m_addActionEdit->setText(QString());

    addNewItem(summary);
}

#include "actionlisteditorpage.moc"