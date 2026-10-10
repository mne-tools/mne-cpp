//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_datamanager.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze data manager: selecting and deleting items in its tree.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/datamanager/datamanager.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/event.h>
#include <anShared/Management/eventmanager.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <QtTest>
#include <QDockWidget>
#include <QTreeView>

#include <memory>

using namespace DATAMANAGERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeDataManager : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void selectsAndDeletesRecordings();
    void cleanupTestCase();
};

//=============================================================================================================

void TestAnalyzeDataManager::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    EventManager::startEventHandling();
}

//=============================================================================================================

void TestAnalyzeDataManager::selectsAndDeletesRecordings()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    DataManager plugin;
    plugin.setGlobalData(data);
    plugin.init();
    std::unique_ptr<QDockWidget> dock(plugin.getControl());
    dock->show();
    auto* tree = dock->findChild<QTreeView*>();
    QVERIFY(tree);

    QList<QSharedPointer<AbstractModel>> selected;
    QList<QSharedPointer<AbstractModel>> removed;
    Communicator listener(QVector<EVENT_TYPE>{SELECTED_MODEL_CHANGED, MODEL_REMOVED});
    connect(&listener, &Communicator::receivedEvent, this, [&](const QSharedPointer<Event> e) {
        (e->getType() == SELECTED_MODEL_CHANGED ? selected : removed) << e->getData().value<QSharedPointer<AbstractModel>>();
    });

    auto raw = data->loadModel<FiffRawViewModel>(rawPath);
    QModelIndex rawIndex;
    for (QStandardItem* item : data->getDataModel()->findItems(raw->getModelName(), Qt::MatchRecursive))
        rawIndex = item->index();
    QVERIFY(rawIndex.isValid());

    // Delete with nothing current removes nothing
    tree->setCurrentIndex(QModelIndex());
    QTest::keyClick(tree, Qt::Key_Delete);
    QCoreApplication::processEvents();
    QVERIFY(removed.isEmpty());
    QCOMPARE(data->getModelsByType(ANSHAREDLIB_FIFFRAW_MODEL).size(), 1);

    // Clicking the recording selects it for the other plugins
    tree->expandAll();
    tree->setCurrentIndex(rawIndex);
    QTRY_VERIFY_WITH_TIMEOUT(!selected.isEmpty(), 5000);
    QCOMPARE(selected.last(), raw.staticCast<AbstractModel>());

    // Events of the recording live under it; deleting the recording removes both and tells the other plugins
    auto events = QSharedPointer<EventModel>::create(raw);
    data->addModel<EventModel>(events, QStringLiteral("Events"));
    QCOMPARE(data->getModelsByType(ANSHAREDLIB_EVENT_MODEL).size(), 1);
    tree->setCurrentIndex(rawIndex);
    QTest::keyClick(tree, Qt::Key_Delete);
    QTRY_VERIFY_WITH_TIMEOUT(removed.size() >= 2, 5000);
    QVERIFY(removed.contains(raw.staticCast<AbstractModel>()));
    QVERIFY(removed.contains(events.staticCast<AbstractModel>()));
    QVERIFY(data->getModelsByType(ANSHAREDLIB_FIFFRAW_MODEL).isEmpty());
    QVERIFY(data->getModelsByType(ANSHAREDLIB_EVENT_MODEL).isEmpty());
}

//=============================================================================================================

void TestAnalyzeDataManager::cleanupTestCase()
{
    EventManager::stopEventHandling();
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeDataManager)
#include "test_analyze_datamanager.moc"
