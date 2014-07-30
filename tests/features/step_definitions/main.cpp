#include <boost/test/unit_test.hpp>
#include <cucumber-cpp/defs.hpp>

#include <QApplication>
#include <QDebug>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTest>

#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>

#include "akonadi/akonadiartifactqueries.h"
#include "akonadi/akonadidatasourcequeries.h"
#include "akonadi/akonadinoterepository.h"
#include "akonadi/akonaditaskqueries.h"
#include "akonadi/akonaditaskrepository.h"
#include "presentation/applicationmodel.h"
#include "presentation/inboxpagemodel.h"
#include "presentation/querytreemodelbase.h"
#include "presentation/datasourcelistmodel.h"

static int argc = 0;
static QApplication app(argc, 0);

namespace cucumber {
    namespace internal {
        template<>
        inline QString fromString(const std::string& s) {
            return QString::fromUtf8(s.data());
        }
    }
}

using namespace cucumber;

class ZanshinContext : public QObject
{
    Q_OBJECT
public:
    explicit ZanshinContext(QObject *parent = 0)
        : QObject(parent),
          app(0),
          presentation(0),
          proxyModel(new QSortFilterProxyModel(this))
    {
        using namespace Presentation;
        proxyModel->setDynamicSortFilter(true);

        app = new ApplicationModel(new Akonadi::ArtifactQueries(this),
                                   new Akonadi::DataSourceQueries(this),
                                   new Akonadi::TaskQueries(this),
                                   new Akonadi::TaskRepository(this),
                                   new Akonadi::NoteRepository(this),
                                   this);
    }

    ~ZanshinContext()
    {
    }

    void setModel(QAbstractItemModel *model)
    {
        proxyModel->setSourceModel(model);
        proxyModel->setSortRole(Qt::DisplayRole);
        proxyModel->sort(0);
    }

    QAbstractItemModel *model()
    {
        return proxyModel;
    }

    Presentation::ApplicationModel *app;

    QList<QPersistentModelIndex> indices;
    QPersistentModelIndex index;
    QObject *presentation;

private:
    QSortFilterProxyModel *proxyModel;
};

GIVEN("^I got a task data source list model$") {
    ScenarioScope<ZanshinContext> context;
    context->setModel(context->app->taskSourcesModel());
    QTest::qWait(500);
}

GIVEN("^I got a note data source list model$") {
    ScenarioScope<ZanshinContext> context;
    context->setModel(context->app->noteSourcesModel());
    QTest::qWait(500);
}

GIVEN("^I'm looking at the inbox view$") {
    ScenarioScope<ZanshinContext> context;
    context->presentation = context->app->currentPage();
    QTest::qWait(500);
}

GIVEN("^an item named \"(.+)\" in the central list$") {
    REGEX_PARAM(QString, itemName);

    ScenarioScope<ZanshinContext> context;
    QTest::qWait(500);

    auto model = context->presentation->property("centralListModel").value<QAbstractItemModel*>();
    QTest::qWait(500);
    context->setModel(model);

    for (int row = 0; row < context->model()->rowCount(); row++) {
        QModelIndex index = context->model()->index(row, 0);
        if (index.data().toString() == itemName) {
            context->index = index;
            return;
        }
    }

    qDebug() << "Couldn't find an item named" << itemName;
    BOOST_REQUIRE(false);
}


WHEN("^I look at the central list$") {
    ScenarioScope<ZanshinContext> context;

    auto model = context->presentation->property("centralListModel").value<QAbstractItemModel*>();
    QTest::qWait(500);
    context->setModel(model);

    for (int row = 0; row < context->model()->rowCount(); row++) {
        context->indices << context->model()->index(row, 0);
    }
}

WHEN("^I check the item$") {
    ScenarioScope<ZanshinContext> context;
    context->model()->setData(context->index, Qt::Checked, Qt::CheckStateRole);
}

WHEN("^I remove the item$") {
    ScenarioScope<ZanshinContext> context;
    BOOST_REQUIRE(QMetaObject::invokeMethod(context->presentation, "removeItem", Q_ARG(QModelIndex, context->index)));
}

WHEN("^I add a task named \"(.+)\"$") {
    REGEX_PARAM(QString, itemName);

    ScenarioScope<ZanshinContext> context;
    BOOST_REQUIRE(QMetaObject::invokeMethod(context->presentation, "addTask", Q_ARG(QString, itemName)));
}

WHEN("^I list the model$") {
    ScenarioScope<ZanshinContext> context;
    for (int row = 0; row < context->model()->rowCount(); row++) {
        context->indices << context->model()->index(row, 0);
    }
}

WHEN("^the setting key (\\S+) changes to (\\d+)$") {
    REGEX_PARAM(QString, keyName);
    REGEX_PARAM(qint64, id);

    KConfigGroup config(KGlobal::config(), "General");
    config.writeEntry(keyName, id);
}

WHEN("^the user changes the default (\\S+) data source to (.*)$") {
    REGEX_PARAM(QString, sourceType);
    REGEX_PARAM(QString, sourceName);

    ScenarioScope<ZanshinContext> context;
    auto sourcesResult = sourceType == "task" ? context->app->taskSources()
                       : sourceType == "note" ? context->app->noteSources()
                       : Domain::QueryResult<Domain::DataSource::Ptr>::Ptr();
    QTest::qWait(500);
    auto sources = sourcesResult->data();
    auto source = *std::find_if(sources.begin(), sources.end(),
                                [=] (const Domain::DataSource::Ptr &source) {
                                    return source->name() == sourceName;
                                });


    auto propertyName = sourceType == "task" ? "defaultTaskDataSource"
                      : sourceType == "note" ? "defaultNoteDataSource"
                      : 0;
    context->app->setProperty(propertyName, QVariant::fromValue(source));
}


THEN("^the list is") {
    TABLE_PARAM(tableParam);

    ScenarioScope<ZanshinContext> context;
    auto roleNames = context->model()->roleNames();
    QSet<int> usedRoles;

    QStandardItemModel referenceModel;
    for (const auto row : tableParam.hashes()) {
        QStandardItem *item = new QStandardItem;
        for (const auto it : row) {
            const QByteArray roleName = it.first.data();
            const QString value = QString::fromUtf8(it.second.data());
            const int role = roleNames.key(roleName, -1);
            BOOST_REQUIRE(role != -1);
            item->setData(value, role);
            usedRoles.insert(role);
        }
        referenceModel.appendRow(item);
    }

    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&referenceModel);
    proxy.setSortRole(Qt::DisplayRole);
    proxy.sort(0);

    for (int row = 0; row < context->indices.size(); row++) {
        QModelIndex expectedIndex = proxy.index(row, 0);
        QModelIndex resultIndex = context->indices.at(row);

        for (auto role : usedRoles) {
            BOOST_REQUIRE(expectedIndex.data(role).toString() == resultIndex.data(role).toString());
        }
    }
    BOOST_REQUIRE(proxy.rowCount() == context->indices.size());
}

THEN("^The list contains \"(.+)\"$") {
    REGEX_PARAM(QString, itemName);

    ScenarioScope<ZanshinContext> context;
    for (int row = 0; row < context->indices.size(); row++) {
        if (context->indices.at(row).data().toString() == itemName)
            return;
    }

    BOOST_REQUIRE(false);
}

THEN("^The list does not contain \"(.+)\"$") {
    REGEX_PARAM(QString, itemName);

    ScenarioScope<ZanshinContext> context;
    for (int row = 0; row < context->indices.size(); row++) {
        BOOST_REQUIRE(context->indices.at(row).data().toString() != itemName);
    }
}

THEN("^The task corresponding to the item is done$") {
    ScenarioScope<ZanshinContext> context;
    auto artifact = context->index.data(Presentation::QueryTreeModelBase::ObjectRole).value<Domain::Artifact::Ptr>();
    BOOST_REQUIRE(artifact);
    auto task = artifact.dynamicCast<Domain::Task>();
    BOOST_REQUIRE(task);
    BOOST_REQUIRE(task->isDone());
}

THEN("^the default (\\S+) data source is (.*)$") {
    REGEX_PARAM(QString, sourceType);
    REGEX_PARAM(QString, expectedName);

    auto propertyName = sourceType == "task" ? "defaultTaskDataSource"
                      : sourceType == "note" ? "defaultNoteDataSource"
                      : 0;

    ScenarioScope<ZanshinContext> context;
    auto source = context->app->property(propertyName).value<Domain::DataSource::Ptr>();
    BOOST_REQUIRE(!source.isNull());
    BOOST_REQUIRE(source->name() == expectedName);
}

THEN("^the setting key (\\S+) is (\\d+)$") {
    REGEX_PARAM(QString, keyName);
    REGEX_PARAM(qint64, expectedId);

    KConfigGroup config(KGlobal::config(), "General");
    const qint64 id = config.readEntry(keyName, -1);
    BOOST_REQUIRE(id == expectedId);
}

#include "main.moc"
