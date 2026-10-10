//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     test_analyze_dataloader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.4.0
 * @date     October, 2026
 * @brief    Opens the sample files through the mne_analyze data loader, as `mne_analyze file <path>` does.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <applications/mne_analyze/plugins/dataloader/dataloader.h>

#include <anShared/Management/analyzedata.h>
#include <anShared/Management/event.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/fiffrawviewmodel.h>

#include <fiff/fiff_evoked_set.h>
#include <fiff/fiff_events.h>

#include <QtTest>

#include <memory>

using namespace DATALOADERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================

class TestAnalyzeDataLoader : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void loadsEveryFileKind();
    void attachesEventFilesToTheRecording();
    void savesTheSelectedAverage();
};

//=============================================================================================================

namespace
{

QString sampleFile(const QString& name)
{
    return QCoreApplication::applicationDirPath() + "/../resources/data/mne-cpp-test-data/" + name;
}

void open(DataLoader& loader, const QString& path)
{
    loader.cmdLineStartup({QStringLiteral("file"), path});
}

/** Types sPath into the next file dialog and accepts it. */
void chooseFileInNextDialog(const QString& sPath)
{
    QTimer::singleShot(10, qApp, [sPath]() {
        auto* pDialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!pDialog) {
            chooseFileInNextDialog(sPath);
            return;
        }
        pDialog->setDirectory(QFileInfo(sPath).absolutePath());
        pDialog->findChild<QLineEdit*>(QStringLiteral("fileNameEdit"))->setText(QFileInfo(sPath).fileName());
        static_cast<QDialog*>(pDialog)->accept();
    });
}

QAction* findAction(QMenu* pMenu, const QString& sText)
{
    for (QAction* pAction : pMenu->actions()) {
        if (pAction->text() == sText)
            return pAction;
        if (QAction* pFound = pAction->menu() ? findAction(pAction->menu(), sText) : nullptr)
            return pFound;
    }
    return nullptr;
}

} // namespace

//=============================================================================================================

void TestAnalyzeDataLoader::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    if (!QFile::exists(sampleFile("MEG/sample/sample_audvis_trunc_raw.fif")))
        QSKIP("Sample test data not found");
}

//=============================================================================================================

void TestAnalyzeDataLoader::loadsEveryFileKind()
{
    auto data = QSharedPointer<AnalyzeData>::create();
    DataLoader loader;
    loader.setGlobalData(data);
    loader.init();

    const QList<QPair<QString, MODEL_TYPE>> files{
        {"MEG/sample/sample_audvis_trunc_raw.fif", ANSHAREDLIB_FIFFRAW_MODEL},
        {"MEG/sample/sample_audvis-ave.fif", ANSHAREDLIB_AVERAGING_MODEL},
        {"MEG/sample/sample_audvis-cov.fif", ANSHAREDLIB_NOISE_MODEL},
        {"MEG/sample/all-trans.fif", ANSHAREDLIB_MRICOORD_MODEL},
        {"subjects/sample/bem/sample-5120-bem.fif", ANSHAREDLIB_BEMDATA_MODEL},
    };
    for (const auto& [name, type] : files) {
        open(loader, sampleFile(name));
        const QSharedPointer<AbstractModel> model = data->getModelByPath(sampleFile(name));
        QVERIFY2(model, qPrintable(name));
        QCOMPARE(model->getType(), type);
    }
    QCOMPARE(data->getAllModels().size(), files.size());

    // Unknown kinds load nothing
    open(loader, sampleFile("MEG/sample/data_spectral_connectivity.txt"));
    QCOMPARE(data->getAllModels().size(), files.size());
}

//=============================================================================================================

void TestAnalyzeDataLoader::attachesEventFilesToTheRecording()
{
    QTemporaryDir dir;
    FIFFLIB::FiffEvents events;
    events.events.resize(2, 3);
    events.events << 25800, 0, 1, 26000, 0, 2;
    QFile fif(dir.filePath("triggers-eve.fif"));
    QFile text(dir.filePath("triggers.eve"));
    QVERIFY(events.write_to_fif(fif));
    fif.close();
    QVERIFY(events.write_to_ascii(text, 600.614990234375f));

    auto data = QSharedPointer<AnalyzeData>::create();
    DataLoader loader;
    loader.setGlobalData(data);
    loader.init();

    // An event list opened before any recording has nothing to attach to, but must still load
    open(loader, text.fileName());
    QVERIFY(data->getModelByPath(text.fileName()));

    const QString rawPath = sampleFile("MEG/sample/sample_audvis_trunc_raw.fif");
    open(loader, rawPath);
    auto raw = qSharedPointerCast<FiffRawViewModel>(data->getModelByPath(rawPath));
    QVERIFY(raw);
    loader.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(raw.staticCast<AbstractModel>())));

    // Both event file kinds take the recording and its sampling frequency
    QFile::copy(text.fileName(), dir.filePath("again.eve"));
    for (const QString& path : {dir.filePath("again.eve"), fif.fileName()}) {
        open(loader, path);
        auto model = qSharedPointerCast<EventModel>(data->getModelByPath(path));
        QVERIFY2(model, qPrintable(path));
        QVERIFY2(model->rowCount() == 2, qPrintable(path));
        QCOMPARE(model->getFiffModel(), raw);
        QCOMPARE(model->getSampleFreq(), raw->getFiffInfo()->sfreq);
    }
}

//=============================================================================================================

void TestAnalyzeDataLoader::savesTheSelectedAverage()
{
    auto data = QSharedPointer<AnalyzeData>::create();
    DataLoader loader;
    loader.setGlobalData(data);
    loader.init();
    const QString avePath = sampleFile("MEG/sample/sample_audvis-ave.fif");
    open(loader, avePath);
    loader.handleEvent(QSharedPointer<Event>::create(SELECTED_MODEL_CHANGED, nullptr, QVariant::fromValue(data->getModelByPath(avePath))));

    // File > Save > Save average writes the selected average
    QTemporaryDir dir;
    const QString outPath = dir.filePath("copy-ave.fif");
    std::unique_ptr<QMenu> menu(loader.getMenu());
    QAction* pSave = findAction(menu.get(), QStringLiteral("Save average"));
    QVERIFY(pSave);
    chooseFileInNextDialog(outPath);
    pSave->trigger();
    QVERIFY(QFile::exists(outPath));

    QFile original(avePath);
    QFile copy(outPath);
    const FIFFLIB::FiffEvokedSet expected(original);
    const FIFFLIB::FiffEvokedSet saved(copy);
    QCOMPARE(saved.evoked.size(), expected.evoked.size());
    for (int k = 0; k < expected.evoked.size(); ++k) {
        QCOMPARE(saved.evoked[k].comment, expected.evoked[k].comment);
        QCOMPARE(saved.evoked[k].nave, expected.evoked[k].nave);
        QVERIFY(saved.evoked[k].data.isApprox(expected.evoked[k].data, 1e-6));
    }
}

//=============================================================================================================

QTEST_MAIN(TestAnalyzeDataLoader)
#include "test_analyze_dataloader.moc"
