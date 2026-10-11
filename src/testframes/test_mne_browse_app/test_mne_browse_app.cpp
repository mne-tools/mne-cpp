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
#include <Windows/datawindow.h>
#include <Windows/eventwindow.h>
#include <Models/eventmodel.h>
#include <Windows/mainwindow.h>
#include <Windows/annotationwindow.h>
#include <Models/annotationmodel.h>
#include <Utils/sessionfilter.h>
#include <Models/averagemodel.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <fiff/fiff_cov.h>
#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_tag.h>
#include <mna/mna_io.h>
#include <mna/mna_project.h>

#include <QAction>
#include <QColorDialog>
#include <QComboBox>
#include <QCryptographicHash>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QTimer>

#include <functional>
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
    void computesAndSavesCovarianceLikePython();
    void eventMarkersUseEventTypeColors();
    void savesAndOpensProjects();
    void writesRawCopyButNotOverItsSource();
    void annotatesDraggedRangeLikePython();

private:
    static MainWindow* makeWindow();
};

//=============================================================================================================

namespace
{

/** Runs fn on the next modal widget as soon as it is shown. */
void answerNextModal(const std::function<void(QWidget*)>& fn)
{
    QTimer::singleShot(10, qApp, [fn]() {
        if (QWidget* pModal = QApplication::activeModalWidget()) {
            fn(pModal);
        } else {
            answerNextModal(fn);
        }
    });
}

QAction* findAction(QWidget* window, const QString& text)
{
    for (QAction* action : window->findChildren<QAction*>()) {
        if (action->text() == text)
            return action;
    }
    return nullptr;
}

} // namespace

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

    // Save Evoked (fif)... writes the same averages
    QTemporaryDir dir;
    const QString avePath = dir.filePath("recomputed-ave.fif");
    answerNextModal([avePath](QWidget* pModal) {
        auto* dialog = qobject_cast<QFileDialog*>(pModal);
        dialog->setDirectory(QFileInfo(avePath).absolutePath());
        dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(avePath).fileName());
        static_cast<QDialog*>(dialog)->accept();
    });
    findAction(window, QStringLiteral("Save Evoked (fif)..."))->trigger();
    QFile aveFile(avePath);
    const FIFFLIB::FiffEvokedSet saved(aveFile);
    QCOMPARE(saved.evoked.size(), 2);
    for (int row = 0; row < 2; ++row) {
        QCOMPARE(saved.evoked[row].nave, 6);
        QVERIFY(saved.evoked[row].data.isApprox(model->getEvoked(row)->data, 1e-6));
    }

    // H shows the peak-to-peak histogram of the 12 epochs; the fullest bin spans the plot height
    QImage histogram;
    QString histogramTitle;
    answerNextModal([&histogram, &histogramTitle](QWidget* pModal) {
        histogramTitle = pModal->windowTitle();
        histogram = pModal->findChild<QLabel*>()->pixmap().toImage();
        qobject_cast<QDialog*>(pModal)->accept();
    });
    QTest::keyClick(window->dataWindow(), Qt::Key_H);
    QCOMPARE(histogramTitle, QStringLiteral("Epoch PTP Histogram (12 epochs)"));
    QCOMPARE(histogram.size(), QSize(500, 300));
    bool fullBin = false;
    for (int x = 50; x < histogram.width() - 20; ++x)
        fullBin = fullBin || QColor(histogram.pixel(x, 21)) == QColor(70, 130, 180);
    QVERIFY(fullBin);
    window->hide();
    QCoreApplication::processEvents();
}

//=============================================================================================================

void TestMneBrowseApp::computesAndSavesCovarianceLikePython()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    // Compute Covariance with the dialog's defaults: every event code, -200..0 ms, no baseline, sample mean removed
    QSettings settings;
    settings.remove("MainWindow/Covariance");
    settings.sync();

    auto* window = makeWindow();
    window->resize(1200, 800);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->applyCommandLineOptions(rawPath, QString(), -1.0, -1.0);

    QAction* compute = findAction(window, QStringLiteral("Compute Covariance..."));
    QAction* save = findAction(window, QStringLiteral("Save Covariance (fif)..."));
    QVERIFY(compute && save);
    answerNextModal([](QWidget* pModal) {
        qobject_cast<QDialog*>(pModal)->accept();
        answerNextModal([](QWidget* pInfo) { qobject_cast<QDialog*>(pInfo)->accept(); });
    });
    compute->trigger();

    QTemporaryDir dir;
    const QString covPath = dir.filePath("sample-cov.fif");
    answerNextModal([covPath](QWidget* pModal) {
        auto* dialog = qobject_cast<QFileDialog*>(pModal);
        dialog->setDirectory(QFileInfo(covPath).absolutePath());
        dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(covPath).fileName());
        static_cast<QDialog*>(dialog)->accept();
    });
    save->trigger();
    window->hide();
    QCoreApplication::processEvents();

    // Reference values produced by mne.compute_covariance(mne.Epochs(raw, find_events(raw, "STI 014"), all codes,
    // -0.2, 0.0, baseline=None, proj=False), keep_sample_mean=False, method="empirical", rank="full") (mne 1.11.0)
    QFile covFile(covPath);
    const FIFFLIB::FiffCov cov(covFile);
    QVERIFY(!cov.isEmpty());
    QCOMPARE(cov.nfree, 1524);
    const int meg = static_cast<int>(cov.names.indexOf("MEG1332"));
    const int eeg = static_cast<int>(cov.names.indexOf("EEG021"));
    QVERIFY(meg >= 0 && eeg >= 0);
    const double expected[3] = {4.770659745331084e-30, 3.182089184120055e-17, 1.7621560460231146e-24};
    const double got[3] = {cov.data(meg, meg), cov.data(eeg, eeg), cov.data(meg, eeg)};
    for (int k = 0; k < 3; ++k)
        QVERIFY2(std::abs(got[k] - expected[k]) < 1e-5 * std::abs(expected[k]), qPrintable(QString::number(got[k], 'g', 17)));
}

void TestMneBrowseApp::eventMarkersUseEventTypeColors()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");
    QSettings().remove("EventDesignParameters");

    auto* window = makeWindow();
    window->resize(1200, 800);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->applyCommandLineOptions(rawPath, QString(), -1.0, -1.0);
    DataWindow* dataWindow = window->dataWindow();
    QTRY_VERIFY_WITH_TIMEOUT(!dataWindow->stimEvents().isEmpty(), 10000);

    // Markers of the trigger channel are drawn in the colours the event list shows
    EventModel* eventModel = window->eventWindow()->getEventModel();
    for (const auto& marker : dataWindow->stimEvents())
        QCOMPARE(marker.color, eventModel->getEventTypeColors().value(marker.type));

    // A colour picked in the event window recolours the markers of that type
    const int type = dataWindow->stimEvents().first().type;
    const QColor picked(12, 34, 56);
    eventModel->addNewEventType(QString::number(type), picked);
    dataWindow->updateDataTableViews();
    for (const auto& marker : dataWindow->stimEvents()) {
        if (marker.type == type)
            QCOMPARE(marker.color, picked);
    }

    // Adding a type in the event window: cancelling the colour dialog adds nothing
    EventWindow* eventWindow = window->eventWindow();
    eventWindow->findChild<QSpinBox*>("m_spinBox_addEventType")->setValue(77);
    auto* addButton = eventWindow->findChild<QPushButton*>("m_pushButton_addEventType");
    answerNextModal([](QWidget* dialog) { qobject_cast<QDialog*>(dialog)->reject(); });
    addButton->click();
    QVERIFY(!eventModel->getEventTypeColors().contains(77));
    answerNextModal([](QWidget* dialog) {
        qobject_cast<QColorDialog*>(dialog)->setCurrentColor(QColor(7, 8, 9));
        qobject_cast<QDialog*>(dialog)->accept();
    });
    addButton->click();
    QCOMPARE(eventModel->getEventTypeColors().value(77), QColor(7, 8, 9));
    QSettings().remove("EventDesignParameters");
}

//=============================================================================================================

void TestMneBrowseApp::writesRawCopyButNotOverItsSource()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    QTemporaryDir dir;
    QVERIFY(QDir(dir.path()).mkdir("sub"));
    const QString sourcePath = dir.filePath("raw.fif");
    QVERIFY(QFile::copy(rawPath, sourcePath));
    const auto fileHash = [](const QString& path) {
        QFile file(path);
        QCryptographicHash hash(QCryptographicHash::Sha256);
        return file.open(QIODevice::ReadOnly) && hash.addData(&file) ? hash.result() : QByteArray();
    };
    const QByteArray sourceHash = fileHash(sourcePath);

    auto* window = makeWindow();
    window->resize(1200, 800);
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));
    window->applyCommandLineOptions(dir.filePath("sub/../raw.fif"), QString(), -1.0, -1.0);
    QAction* write = window->findChild<QAction*>(QStringLiteral("m_writeAction"));
    QVERIFY(write);
    const auto chooseFile = [](const QString& path) {
        answerNextModal([path](QWidget* pModal) {
            auto* dialog = qobject_cast<QFileDialog*>(pModal);
            dialog->setDirectory(QFileInfo(path).absolutePath());
            dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(path).fileName());
            static_cast<QDialog*>(dialog)->accept();
        });
    };

    // Writing over the file the data is read from is refused, however the path was spelled
    QString refusal;
    answerNextModal([&refusal, sourcePath](QWidget* pModal) {
        auto* dialog = qobject_cast<QFileDialog*>(pModal);
        dialog->setDirectory(QFileInfo(sourcePath).absolutePath());
        dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(sourcePath).fileName());
        // The dialog first asks to replace the existing file, then the app refuses
        answerNextModal([&refusal](QWidget* pReplace) {
            qobject_cast<QMessageBox*>(pReplace)->button(QMessageBox::Yes)->click();
            answerNextModal([&refusal](QWidget* pBox) {
                refusal = qobject_cast<QMessageBox*>(pBox)->text();
                qobject_cast<QDialog*>(pBox)->accept();
            });
        });
        static_cast<QDialog*>(dialog)->accept();
    });
    write->trigger();
    QVERIFY2(refusal.contains("currently loading the data from"), qPrintable(refusal));
    QCOMPARE(fileHash(sourcePath), sourceHash);

    // A copy holds the same data
    const QString copyPath = dir.filePath("copy_raw.fif");
    chooseFile(copyPath);
    write->trigger();

    // With an EEG-only session filter the copy goes through the filtered writer; MEG and EOG stay untouched
    const QString filteredPath = dir.filePath("filtered_raw.fif");
    window->dataWindow()->setUserDefinedFilter(QSharedPointer<MNEBROWSE::SessionFilter>::create(
        QStringLiteral("lp"), MNEBROWSE::SessionFilter::DesignMethod::Butterworth, MNEBROWSE::SessionFilter::FilterType::LowPass,
        4, 40.0, 0.0, 5.0, window->dataWindow()->fiffInfo()->sfreq, QStringLiteral("EEG")));
    chooseFile(filteredPath);
    write->trigger();
    window->hide();
    QCoreApplication::processEvents();

    QFile sourceFile(sourcePath);
    FIFFLIB::FiffRawData source(sourceFile);
    Eigen::MatrixXd sourceData, times;
    QVERIFY(source.read_raw_segment(sourceData, times));
    const int meg = static_cast<int>(source.info.ch_names.indexOf("MEG1332"));
    const int eeg = static_cast<int>(source.info.ch_names.indexOf("EEG021"));
    const int eog = static_cast<int>(source.info.ch_names.indexOf("EOG061"));
    QVERIFY(meg >= 0 && eeg >= 0 && eog >= 0);
    for (const QString& path : {copyPath, filteredPath}) {
        QFile file(path);
        FIFFLIB::FiffRawData written(file);
        QCOMPARE(written.info.nchan, source.info.nchan);
        QCOMPARE(written.first_samp, source.first_samp);
        QCOMPARE(written.last_samp, source.last_samp);
        Eigen::MatrixXd writtenData;
        QVERIFY(written.read_raw_segment(writtenData, times));
        QVERIFY2(writtenData.row(meg).isApprox(sourceData.row(meg), 1e-6), qPrintable(path));
        QVERIFY2(writtenData.row(eog).isApprox(sourceData.row(eog), 1e-6), qPrintable(path));
        QVERIFY2(writtenData.row(eeg).isApprox(sourceData.row(eeg), 1e-6) == (path == copyPath), qPrintable(path));
    }
}

//=============================================================================================================

void TestMneBrowseApp::annotatesDraggedRangeLikePython()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    QTemporaryDir dir;
    const QString annotPath = dir.filePath("drag-annot.fif");
    const auto chooseFile = [](const QString& path) {
        answerNextModal([path](QWidget* pModal) {
            auto* dialog = qobject_cast<QFileDialog*>(pModal);
            dialog->setDirectory(QFileInfo(path).absolutePath());
            dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(path).fileName());
            static_cast<QDialog*>(dialog)->accept();
        });
    };
    const auto openWindow = [rawPath]() {
        auto* window = makeWindow();
        window->resize(1200, 800);
        window->show();
        window->applyCommandLineOptions(rawPath, QString(), -1.0, -1.0);
        return window;
    };

    // A right-to-left drag over samples 13500..13800, labelled in the dialog
    auto* first = openWindow();
    answerNextModal([](QWidget* pModal) {
        pModal->findChild<QComboBox*>()->setCurrentText(QStringLiteral(" BAD_drag "));
        qobject_cast<QDialog*>(pModal)->accept();
    });
    emit first->dataWindow()->annotationRangeSelected(13800, 13500);
    chooseFile(annotPath);
    findAction(first, QStringLiteral("Save Annotations..."))->trigger();
    first->hide();

    // Reference values produced by t = raw.times[[600, 900]]; raw.set_annotations(mne.Annotations([t[0]],
    // [t[1] - t[0]], ["BAD_drag"])) (mne 1.11.0): onsets count from orig_time = meas_date
    QFile annotFile(annotPath);
    FIFFLIB::FiffStream stream(&annotFile);
    QVERIFY(stream.open());
    const QList<FIFFLIB::FiffDirNode::SPtr> blocks = stream.dirtree()->dir_tree_find(3810);
    QCOMPARE(blocks.size(), 1);
    QMap<int, QVector<float>> times;
    for (const auto& entry : blocks.first()->dir) {
        FIFFLIB::FiffTag::UPtr tag;
        if ((entry->kind == 3568 || entry->kind == 3569) && stream.read_tag(tag, entry->pos))
            times[entry->kind] = QVector<float>(tag->toFloat(), tag->toFloat() + tag->size() / 4);
    }
    annotFile.close();
    QCOMPARE(times[3568].size(), 1);
    QVERIFY2(std::abs(times[3568][0] - 44.95392282905339) < 1e-5, qPrintable(QString::number(times[3568][0], 'g', 10)));
    QVERIFY(std::abs(times[3569][0] - times[3568][0] - 0.9989760657919393) < 1e-5);

    // Loading the file again restores the same span and label
    auto* second = openWindow();
    chooseFile(annotPath);
    findAction(second, QStringLiteral("Load Annotations..."))->trigger();
    const QVector<MNEBROWSE::AnnotationSpanData> spans = second->findChild<MNEBROWSE::AnnotationWindow*>()->getAnnotationModel()->getAnnotationSpans();
    second->hide();
    QCoreApplication::processEvents();
    QCOMPARE(spans.size(), 1);
    QCOMPARE(spans[0].startSample, 13500);
    QCOMPARE(spans[0].endSample, 13800);
    QCOMPARE(spans[0].label, QStringLiteral("BAD_drag"));
}

//=============================================================================================================

void TestMneBrowseApp::savesAndOpensProjects()
{
    const QString rawPath = QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif";
    if (!QFile::exists(rawPath))
        QSKIP("Sample test data not found");

    QTemporaryDir dir;
    const auto chooseFile = [](const QString& path) {
        answerNextModal([path](QWidget* pModal) {
            auto* dialog = qobject_cast<QFileDialog*>(pModal);
            dialog->setDirectory(QFileInfo(path).absolutePath());
            dialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(path).fileName());
            static_cast<QDialog*>(dialog)->accept();
        });
    };
    const auto openWindow = []() {
        auto* window = makeWindow();
        window->resize(1200, 800);
        window->show();
        return window;
    };
    QFile rawFile(rawPath);
    const int nchan = FIFFLIB::FiffRawData(rawFile).info.nchan;

    // A binary project embeds the recording
    auto* first = openWindow();
    QVERIFY(QTest::qWaitForWindowExposed(first));
    first->applyCommandLineOptions(rawPath, QString(), -1.0, -1.0);
    const QString mnxPath = dir.filePath("study.mnx");
    chooseFile(mnxPath);
    findAction(first, QStringLiteral("Save Project (.mna/.mnx)..."))->trigger();
    const MNALIB::MnaProject mnx = MNALIB::MnaIO::read(mnxPath);
    QCOMPARE(mnx.subjects.size(), 1);
    const MNALIB::MnaFileRef rawRef = mnx.subjects[0].sessions[0].recordings[0].files.first();
    QCOMPARE(rawRef.role, MNALIB::MnaFileRole::Raw);
    QVERIFY(rawRef.embedded);
    QCOMPARE(rawRef.data.size(), QFileInfo(rawPath).size());
    first->hide();

    // Opened again it shows the same recording, extracted from the project
    auto* second = openWindow();
    QVERIFY(QTest::qWaitForWindowExposed(second));
    chooseFile(mnxPath);
    findAction(second, QStringLiteral("Open Project (.mna/.mnx)..."))->trigger();
    QVERIFY(second->dataWindow()->fiffInfo());
    QCOMPARE(second->dataWindow()->fiffInfo()->nchan, nchan);

    // Saved as a JSON project, the recording must live next to the project, not in the session's temp dir
    const QString mnaPath = dir.filePath("study.mna");
    chooseFile(mnaPath);
    findAction(second, QStringLiteral("Save Project (.mna/.mnx)..."))->trigger();
    second->hide();
    const MNALIB::MnaProject mna = MNALIB::MnaIO::read(mnaPath);
    const MNALIB::MnaFileRef mnaRaw = mna.subjects[0].sessions[0].recordings[0].files.first();
    QVERIFY(!mnaRaw.embedded);
    const QString referenced = QDir(dir.path()).absoluteFilePath(mnaRaw.path);
    QVERIFY2(QFileInfo(referenced).absolutePath() == QFileInfo(mnaPath).absolutePath(), qPrintable(mnaRaw.path));
    QCOMPARE(QFileInfo(referenced).size(), QFileInfo(rawPath).size());

    auto* third = openWindow();
    QVERIFY(QTest::qWaitForWindowExposed(third));
    chooseFile(mnaPath);
    findAction(third, QStringLiteral("Open Project (.mna/.mnx)..."))->trigger();
    QVERIFY(third->dataWindow()->fiffInfo());
    QCOMPARE(third->dataWindow()->fiffInfo()->nchan, nchan);
    third->hide();
    QCoreApplication::processEvents();
}

QTEST_MAIN(TestMneBrowseApp)
#include "test_mne_browse_app.moc"