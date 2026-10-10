//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_mne_browse_app.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     August, 2026
 * @brief    Offscreen application smoke tests for mne_browse.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <Windows/averagewindow.h>
#include <Windows/mainwindow.h>
#include <Models/averagemodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QAction>
#include <QDockWidget>
#include <QImage>
#include <QMenuBar>
#include <QPainter>
#include <QRhiWidget>
#include <QStandardPaths>
#include <QtTest>

//=============================================================================================================

using namespace MNEBROWSE;

class TestMneBrowseApp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void constructsAndRendersMainWindow();
    void recomputesEvokedLikePython();

private:
    static MainWindow* makeWindow();
};

//=============================================================================================================

void TestMneBrowseApp::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("mne-cpp-tests");
    QCoreApplication::setApplicationName("test_mne_browse_app");
}

//=============================================================================================================

MainWindow* TestMneBrowseApp::makeWindow()
{
    // The offscreen QRhi backend cannot safely destroy this widget tree, so keep it process-owned.
    auto* window = new MainWindow;
    for (QRhiWidget* rhiWidget : window->findChildren<QRhiWidget*>())
        rhiWidget->setApi(QRhiWidget::Api::Null);
    return window;
}

//=============================================================================================================

void TestMneBrowseApp::constructsAndRendersMainWindow()
{
    auto* window = makeWindow();
    window->resize(1200, 800);
    window->show();
    const bool exposed = QTest::qWaitForWindowExposed(window);
    const bool hasRawModel = window->rawModel() != nullptr;
    const qsizetype dockCount = window->findChildren<QDockWidget*>().size();
    const qsizetype menuCount = window->menuBar()->actions().size();

    QImage image(window->size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    window->render(&painter);
    painter.end();

    QImage blank(image.size(), image.format());
    blank.fill(Qt::transparent);
    const bool rendered = !image.isNull() && image.constBits() != nullptr && image != blank;

    window->hide();
    QCoreApplication::processEvents();

    QVERIFY(exposed);
    QVERIFY(hasRawModel);
    QVERIFY(dockCount >= 5);
    QVERIFY(menuCount >= 4);
    QVERIFY(rendered);
}

//=============================================================================================================

void TestMneBrowseApp::recomputesEvokedLikePython()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    // The setup "Recompute Last Evoked" reuses: codes 1 and 3, -100..300 ms, baseline -100..0 ms, no rejection
    QSettings settings;
    settings.setValue("MainWindow/Averaging/eventCodes", QStringList{"1", "3"});
    settings.setValue("MainWindow/Averaging/preStimMs", 100);
    settings.setValue("MainWindow/Averaging/postStimMs", 300);
    settings.setValue("MainWindow/Averaging/applyBaseline", true);
    settings.setValue("MainWindow/Averaging/baselineFromMs", -100);
    settings.setValue("MainWindow/Averaging/baselineToMs", 0);
    settings.setValue("MainWindow/Averaging/dropRejected", false);
    settings.sync();

    auto* window = makeWindow();
    window->resize(1200, 800);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->applyCommandLineOptions(rawPath, QString(), -1.0, -1.0);

    // No events are loaded, so the run detects them on STI 014 like mne.find_events
    QAction* recompute = nullptr;
    for (QAction* action : window->findChildren<QAction*>()) {
        if (action->text() == QStringLiteral("Recompute Last Evoked"))
            recompute = action;
    }
    QVERIFY(recompute);
    recompute->trigger();
    QCoreApplication::processEvents();

    // Reference values produced by mne.Epochs(raw, find_events(raw, "STI 014"), code, -0.1, 0.3,
    // baseline=(-0.1, 0), proj=False, reject=None).average(picks="all") (mne 1.11.0)
    AverageModel* model = window->findChild<AverageWindow*>()->getAverageModel();
    QVERIFY(model->isFileLoaded());
    struct Expected
    {
        double megSum, eegSum, eeg60;
    };
    const Expected expected[2] = {{5.0567231986985656e-14, -6.734846534962558e-08, -4.282182180774749e-09},
                                  {-6.863652119640187e-14, 3.3627187038465186e-08, -2.481697357232196e-11}};
    for (int row = 0; row < 2; ++row) {
        const FIFFLIB::FiffEvoked* evoked = model->getEvoked(row);
        QVERIFY(evoked);
        QCOMPARE(evoked->nave, 6);
        QCOMPARE(evoked->data.cols(), Eigen::Index(121));
        QVERIFY(std::abs(evoked->times(0) + 0.09989760657919393) < 1e-6);
        const int meg = static_cast<int>(evoked->info.ch_names.indexOf("MEG1332"));
        const int eeg = static_cast<int>(evoked->info.ch_names.indexOf("EEG021"));
        QVERIFY(meg >= 0 && eeg >= 0);
        QVERIFY2(std::abs(evoked->data.row(meg).sum() - expected[row].megSum) < 1e-5 * std::abs(expected[row].megSum),
                 qPrintable(QString::number(evoked->data.row(meg).sum(), 'g', 17)));
        QVERIFY2(std::abs(evoked->data.row(eeg).sum() - expected[row].eegSum) < 1e-5 * std::abs(expected[row].eegSum),
                 qPrintable(QString::number(evoked->data.row(eeg).sum(), 'g', 17)));
        QVERIFY(std::abs(evoked->data(eeg, 60) - expected[row].eeg60) < 1e-5 * std::abs(expected[row].eeg60));
    }
    window->hide();
    QCoreApplication::processEvents();
}

//=============================================================================================================

QTEST_MAIN(TestMneBrowseApp)
#include "test_mne_browse_app.moc"