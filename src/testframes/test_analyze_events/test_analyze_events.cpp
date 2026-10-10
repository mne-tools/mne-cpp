//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_events.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Detects the sample triggers through the mne_analyze events panel and compares them with mne.find_events.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/events/events.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <disp/viewers/triggerdetectionview.h>

#include <QtTest>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QSemaphore>
#include <QSpinBox>
#include <QTableView>
#include <QThreadPool>

#include <map>
#include <memory>
#include <vector>

using namespace EVENTSPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeEvents : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void detectsTriggersLikePython();
};

//=============================================================================================================

void TestAnalyzeEvents::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("MNE-CPP-Test"));
    QCoreApplication::setApplicationName(QStringLiteral("test_analyze_events"));
}

//=============================================================================================================

void TestAnalyzeEvents::detectsTriggersLikePython()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    Events plugin;
    plugin.setGlobalData(data);
    plugin.init();
    std::unique_ptr<QDockWidget> dock(plugin.getControl());

    auto raw = data->loadModel<FiffRawViewModel>(rawPath);
    QVERIFY(raw);
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));
    QSharedPointer<EventModel> events = raw->hasEventModel() ? raw->getEventModel() : QSharedPointer<EventModel>();
    if (!events) {
        for (const auto& model : data->getModelsByType(ANSHAREDLIB_EVENT_MODEL))
            events = qSharedPointerCast<EventModel>(model);
    }
    QVERIFY(events);

    // The trigger panel: STI014, threshold 1, Detect
    DISPLIB::TriggerDetectionView* view = nullptr;
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (auto* v = qobject_cast<DISPLIB::TriggerDetectionView*>(w))
            view = v;
    }
    QVERIFY(view);
    auto* channel = view->findChild<QComboBox*>(QStringLiteral("m_comboBox_triggerChannels"));
    QVERIFY(channel && channel->findText(QStringLiteral("STI014")) >= 0);
    channel->setCurrentText(QStringLiteral("STI014"));
    view->findChild<QDoubleSpinBox*>(QStringLiteral("m_doubleSpinBox_detectionThresholdFirst"))->setValue(1.0);
    view->findChild<QSpinBox*>(QStringLiteral("m_spinBox_detectionThresholdSecond"))->setValue(0);

    // The detection runs on a worker; hold the pool so it starts only after the click has returned
    QThreadPool* pool = QThreadPool::globalInstance();
    const int maxThreads = pool->maxThreadCount();
    pool->setMaxThreadCount(1);
    QSemaphore gate;
    pool->start([&gate] { gate.acquire(); });
    view->findChild<QPushButton*>(QStringLiteral("m_pushButton_DetectTriggers"))->click();
    QVERIFY(channel->count() > 1);
    channel->setCurrentIndex(channel->currentIndex() == 0 ? 1 : 0); // the user picks another channel meanwhile
    gate.release();
    QTRY_VERIFY_WITH_TIMEOUT(events->getNumberOfGroups() >= 6, 10000);
    pool->waitForDone();
    pool->setMaxThreadCount(maxThreads);

    // mne.find_events(raw, "STI 014") (mne 1.11.0): one event per onset, grouped by code
    const std::map<int, std::vector<int>> expected{
        {1, {14385, 15225, 16050, 16856, 17714, 18503}},
        {2, {13988, 14826, 15620, 16467, 17266, 18105}},
        {3, {14172, 15012, 15832, 16662, 17478, 18288}},
        {4, {14609, 15419, 16259, 17925, 18730}},
        {5, {17044}},
        {32, {17324}},
    };
    const auto groups = events->getGroupsToDisplay();
    std::map<int, std::vector<int>> found;
    for (const auto& group : *groups) {
        const QString name = QString::fromStdString(group.name);
        if (!name.startsWith(QStringLiteral("STI014_")))
            continue;
        events->clearGroupSelection();
        events->addToSelectedGroups(group.id);
        const Eigen::MatrixXi matrix = events->getEventMatrix();
        std::vector<int>& samples = found[name.mid(7).toInt()];
        for (int k = 0; k < matrix.rows(); ++k) {
            samples.push_back(matrix(k, 0));
            QCOMPARE(matrix(k, 2), name.mid(7).toInt());
        }
        std::sort(samples.begin(), samples.end());
    }
    QCOMPARE(found.size(), expected.size());
    for (const auto& [code, samples] : expected)
        QVERIFY2(found[code] == samples, qPrintable(QStringLiteral("code %1: %2 events").arg(code).arg(found[code].size())));

    // Selecting the recording again shows its detected events, not a new empty list
    const auto eventModelCount = data->getModelsByType(ANSHAREDLIB_EVENT_MODEL).size();
    plugin.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));
    QCOMPARE(data->getModelsByType(ANSHAREDLIB_EVENT_MODEL).size(), eventModelCount);
    QCOMPARE(qobject_cast<EventModel*>(dock->findChild<QTableView*>(QStringLiteral("m_tableView_eventTableView"))->model()), events.data());

    // The panel's check boxes reach the viewer and the model, once each, also after the model was set again
    auto* activate = dock->findChild<QCheckBox*>(QStringLiteral("m_checkBox_activateEvents"));
    auto* selectedOnly = dock->findChild<QCheckBox*>(QStringLiteral("m_checkBox_showSelectedEventsOnly"));
    QVERIFY(activate && selectedOnly);
    QSignalSpy toggled(dock->widget(), SIGNAL(activeEventsChecked(int)));
    activate->setChecked(!activate->isChecked());
    QCOMPARE(toggled.size(), 1);
    QCOMPARE(toggled.first().first().toInt(), static_cast<int>(activate->checkState()));
    auto* shown = qobject_cast<EventModel*>(dock->findChild<QTableView*>(QStringLiteral("m_tableView_eventTableView"))->model());
    QVERIFY(shown);
    selectedOnly->setChecked(true);
    QCOMPARE(shown->getShowSelected(), static_cast<int>(Qt::Checked));
}

//=============================================================================================================
// MAIN
//=============================================================================================================

QTEST_MAIN(TestAnalyzeEvents)
#include "test_analyze_events.moc"
