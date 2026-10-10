//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_averaging.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Drives the mne_analyze averaging plugin as a user does and compares its averages with mne-python.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/averaging/averaging.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <disp/viewers/averagingsettingsview.h>

#include <fiff/fiff_evoked_set.h>

#include <QtTest>
#include <QDockWidget>

#include <cmath>
#include <memory>

using namespace AVERAGINGPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeAveraging : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void averagesEveryEventCodeLikePython();
};

//=============================================================================================================

void TestAnalyzeAveraging::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("mne-cpp-tests");
    QCoreApplication::setApplicationName("test_analyze_averaging");
}

//=============================================================================================================

void TestAnalyzeAveraging::averagesEveryEventCodeLikePython()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    auto data = QSharedPointer<AnalyzeData>::create();
    auto plugin = std::make_unique<Averaging>();
    plugin->setGlobalData(data);
    plugin->init();
    std::unique_ptr<QWidget> view(plugin->getView());
    std::unique_ptr<QDockWidget> control(plugin->getControl());

    // The raw file with the auditory (3) and visual (1) events mne.find_events(raw, "STI 014") finds
    auto raw = QSharedPointer<FiffRawViewModel>::create(rawPath);
    auto events = QSharedPointer<EventModel>::create(raw);
    raw->setEventModel(events);
    events->addGroup("triggers", Qt::red);
    const int samples[][2] = {{14172, 3}, {14385, 1}, {15012, 3}, {15225, 1}, {15832, 3}, {16050, 1}, {16662, 3}, {16856, 1}, {17478, 3}, {17714, 1}, {18288, 3}, {18503, 1}};
    for (const auto& event : samples)
        events->addEventWithCode(event[0], event[1]);
    plugin->handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));

    // -100..300 ms with a -100..0 ms baseline, set in the control panel
    auto* settings = control->findChild<DISPLIB::AveragingSettingsView*>();
    QVERIFY(settings);
    emit settings->changePreStim(100);
    emit settings->changePostStim(300);
    emit settings->changeBaselineFrom(-100);
    emit settings->changeBaselineTo(0);
    emit settings->changeBaselineActive(true);
    emit settings->calculateAverage(true);

    QSharedPointer<AveragingDataModel> average;
    QTRY_VERIFY_WITH_TIMEOUT(
        [&] {
            for (const auto& model : data->getAllModels()) {
                if (model->getType() == ANSHAREDLIB_AVERAGING_MODEL)
                    average = qSharedPointerCast<AveragingDataModel>(model);
            }
            return !average.isNull();
        }(),
        30000);

    // Reference values produced by mne.Epochs(raw, events, code, -0.1, 0.3, baseline=(-0.1, 0),
    // proj=False, reject=dict(eog=300e-6)).average(picks="all") (mne 1.11.0), codes in ascending order
    struct Expected
    {
        int code;
        double megSum, eegSum, eeg60;
    };
    const Expected expected[2] = {{1, 5.0567231986985656e-14, -6.734846534962558e-08, -4.282182180774749e-09},
                                  {3, -6.863652119640187e-14, 3.3627187038465186e-08, -2.481697357232196e-11}};
    const QSharedPointer<FIFFLIB::FiffEvokedSet> set = average->getEvokedSet();
    QCOMPARE(set->evoked.size(), 2);
    for (int k = 0; k < 2; ++k) {
        const FIFFLIB::FiffEvoked& evoked = set->evoked[k];
        QCOMPARE(evoked.comment, QString::number(expected[k].code));
        QCOMPARE(evoked.nave, 6);
        QCOMPARE(evoked.data.cols(), Eigen::Index(121));
        const int meg = static_cast<int>(evoked.info.ch_names.indexOf("MEG1332"));
        const int eeg = static_cast<int>(evoked.info.ch_names.indexOf("EEG021"));
        QVERIFY(meg >= 0 && eeg >= 0);
        const auto near = [](double a, double b) {
            return std::abs(a - b) < 1e-5 * std::abs(b);
        };
        QVERIFY2(near(evoked.data.row(meg).sum(), expected[k].megSum), qPrintable(QString::number(evoked.data.row(meg).sum(), 'g', 17)));
        QVERIFY2(near(evoked.data.row(eeg).sum(), expected[k].eegSum), qPrintable(QString::number(evoked.data.row(eeg).sum(), 'g', 17)));
        QVERIFY(near(evoked.data(eeg, 60), expected[k].eeg60));
    }
}

//=============================================================================================================

QTEST_MAIN(TestAnalyzeAveraging)
#include "test_analyze_averaging.moc"
