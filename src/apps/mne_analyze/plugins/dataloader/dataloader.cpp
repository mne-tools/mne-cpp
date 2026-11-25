//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2019-2026 MNE-CPP Authors
 *
 * @file     dataloader.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     November, 2019
 * @brief    Definition of the DataLoader class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader.h"

#include <mne/base/log.h>
#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/eventmodel.h>
#include <anShared/Model/averagingdatamodel.h>
#include <anShared/Model/covariancemodel.h>
#include <anShared/Model/mricoordmodel.h>
#include <anShared/Model/dipolefitmodel.h>
#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_dir_node.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/disp_widgets/viewers/progressview.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DATALOADERPLUGIN;
using namespace mne::analyze;

//=============================================================================================================
// DEFINE STATIC HELPERS
//=============================================================================================================

namespace
{

//=============================================================================================================
/**
 * Checks whether a fiff file holds boundary-element surfaces.
 *
 * Only the directory tree is read, so this stays cheap even for the
 * high-resolution head surfaces, which carry a few hundred thousand vertices.
 *
 * @param[in] sFilePath   Path to the fiff file to inspect.
 *
 * @return True if the file contains a BEM or BEM surface block.
 */
bool containsBemSurfaces(const QString& sFilePath)
{
    mne::FiffStream pStream(sFilePath.toStdU16String());

    if (!pStream.open()) {
        return false;
    }

    const bool bHasBem = !pStream.dirtree()->dir_tree_find(FIFFB_BEM).empty() || !pStream.dirtree()->dir_tree_find(FIFFB_BEM_SURF).empty();

    pStream.close();

    return bHasBem;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataLoader::DataLoader()
: m_pProgressViewWidget(std::make_unique<QWidget>())
, m_sSettingsPath("MNEANALYZE/DataLoader")
, m_sLastDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
{
    m_pProgressViewWidget->setWindowFlags(Qt::Window);

    QVBoxLayout* layout = new QVBoxLayout(m_pProgressViewWidget.get());
    auto pProgressView = std::make_unique<mne::disp::ProgressView>(false);
    m_pProgressView = pProgressView.get();
    layout->addWidget(pProgressView.release());
    loadSettings();
}

//=============================================================================================================

DataLoader::~DataLoader()
{
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> DataLoader::clone() const
{
    QSharedPointer<DataLoader> pDataLoaderClone(new DataLoader);
    return pDataLoaderClone;
}

//=============================================================================================================

void DataLoader::init()
{
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void DataLoader::unload()
{
}

//=============================================================================================================

QString DataLoader::getName() const
{
    return "Data Loader";
}

//=============================================================================================================

QMenu* DataLoader::getMenu()
{
    auto pMenuFile = std::make_unique<QMenu>(tr("File"));

    QAction* pActionLoadFile = new QAction(tr("Open File"), pMenuFile.get());
    pActionLoadFile->setStatusTip(tr("Load a data file"));
    connect(pActionLoadFile, &QAction::triggered,
            this, &DataLoader::onLoadFilePressed);

    QAction* pActionLoadScanSession = new QAction(tr("Open MNE Scan Session"), pMenuFile.get());
    pActionLoadScanSession->setStatusTip(tr("Load a recording saved by MNE Scan"));
    connect(pActionLoadScanSession, &QAction::triggered,
            this, &DataLoader::onLoadScanSessionPressed);

    QAction* pActionLoadSubject = new QAction(tr("Open Subject"), pMenuFile.get());
    pActionLoadSubject->setStatusTip(tr("Load a subject folder"));
    connect(pActionLoadSubject, &QAction::triggered,
            this, &DataLoader::onLoadSubjectPressed);

    QAction* pActionLoadSession = new QAction(tr("Open Session"), pMenuFile.get());
    pActionLoadSession->setStatusTip(tr("Load a session folder"));
    connect(pActionLoadSession, &QAction::triggered,
            this, &DataLoader::onLoadSessionPressed);

    QAction* pActionSaveData = new QAction(tr("Save data"), pMenuFile.get());
    pActionSaveData->setStatusTip(tr("Save the selected data file"));
    connect(pActionSaveData, &QAction::triggered, [=, this] {
        onSaveFilePressed(DATA_FILE);
    });

    QAction* pActionSaveAvg = new QAction(tr("Save average"), pMenuFile.get());
    pActionSaveAvg->setStatusTip(tr("Save the selected average"));
    connect(pActionSaveAvg, &QAction::triggered, [=, this] {
        onSaveFilePressed(AVERAGE_FILE);
    });

    QAction* pActionSaveAnn = new QAction(tr("Save events"), pMenuFile.get());
    pActionSaveAnn->setStatusTip(tr("Save the events of the selected data file"));
    connect(pActionSaveAnn, &QAction::triggered, [=, this] {
        onSaveFilePressed(EVENT_FILE);
    });

    QMenu* pBIDSMenu = new QMenu(tr("Load BIDS Folder"), pMenuFile.get());
    pBIDSMenu->addAction(pActionLoadSubject);
    pBIDSMenu->addAction(pActionLoadSession);

    QMenu* pSaveMenu = new QMenu(tr("Save"), pMenuFile.get());
    pSaveMenu->addAction(pActionSaveData);
    pSaveMenu->addAction(pActionSaveAnn);
    pSaveMenu->addAction(pActionSaveAvg);

    pMenuFile->addAction(pActionLoadFile);
    pMenuFile->addAction(pActionLoadScanSession);
    //pMenuFile->addMenu(pBIDSMenu);
    pMenuFile->addMenu(pSaveMenu);
    //pMenuFile->addAction(pActionSave);

    return pMenuFile.release();
}

//=============================================================================================================

QDockWidget* DataLoader::getControl()
{
    return nullptr;
}

//=============================================================================================================

QWidget* DataLoader::getView()
{
    return nullptr;
}

//=============================================================================================================

void DataLoader::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
        case EVENT_TYPE::SELECTED_MODEL_CHANGED:
            onModelChanged(e->getData().value<QSharedPointer<mne::analyze::AbstractModel>>());
            break;
        default:
            mne::Log::warning("[DataLoader::handleEvent] Received an Event that is not handled by switch cases.");
    }
}

//=============================================================================================================

QVector<EVENT_TYPE> DataLoader::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp = {SELECTED_MODEL_CHANGED};

    return temp;
}

//=============================================================================================================

void DataLoader::cmdLineStartup(const QStringList& sArguments)
{
    if (sArguments.size() == 2 && (sArguments.first() == "file" || sArguments.first() == "f")) {
        loadFilePath(sArguments.at(1));
    }
}

//=============================================================================================================

void DataLoader::loadFilePath(const QString& sFilePath)
{
    QFileInfo fileInfo(sFilePath);

    startProgress("Loading " + fileInfo.fileName());

    // Event lists belong to the selected recording, whichever format they are stored in
    const auto attachToRecording = [this](const QSharedPointer<mne::analyze::EventModel>& pModel) {
        if (!pModel || !m_pSelectedModel || !m_pSelectedModel->getFiffInfo())
            return;
        pModel->setFiffModel(m_pSelectedModel);
        pModel->setFirstLastSample(m_pSelectedModel->absoluteFirstSample(), m_pSelectedModel->absoluteLastSample());
        pModel->setSampleFreq(m_pSelectedModel->getFiffInfo()->sfreq);
    };

    if (fileInfo.exists() && (fileInfo.completeSuffix() == "eve")) {
        attachToRecording(m_pAnalyzeData->loadModel<mne::analyze::EventModel>(sFilePath));
    } else if (fileInfo.exists() && fileInfo.suffix() == "dip") {
        m_pAnalyzeData->loadModel<mne::analyze::DipoleFitModel>(sFilePath);
    } else if (fileInfo.exists() && (fileInfo.completeSuffix() == "fif")) {
        if (fileInfo.completeBaseName().endsWith("eve")) {
            attachToRecording(m_pAnalyzeData->loadModel<mne::analyze::EventModel>(sFilePath));
        } else if (fileInfo.completeBaseName().endsWith("bem")) {
            m_pAnalyzeData->loadModel<mne::analyze::BemDataModel>(sFilePath);
        } else if (fileInfo.completeBaseName().endsWith("raw")) {
            m_pAnalyzeData->loadModel<mne::analyze::FiffRawViewModel>(sFilePath);
        } else if (fileInfo.completeBaseName().endsWith("ave")) {
            m_pAnalyzeData->loadModel<mne::analyze::AveragingDataModel>(sFilePath);
        } else if (fileInfo.completeBaseName().endsWith("cov")) {
            m_pAnalyzeData->loadModel<mne::analyze::CovarianceModel>(sFilePath);
        } else if (fileInfo.completeBaseName().endsWith("trans")) {
            m_pAnalyzeData->loadModel<mne::analyze::MriCoordModel>(sFilePath);
        } else if (containsBemSurfaces(sFilePath)) {
            //
            // The naming conventions above miss perfectly valid files: the
            // high-resolution head surfaces shipped with the sample data are
            // called <subject>-head.fif and the solution files end in -sol,
            // so neither reaches the "bem" branch. Fall back to looking at
            // what the file actually contains.
            //
            m_pAnalyzeData->loadModel<mne::analyze::BemDataModel>(sFilePath);
        } else {
            mne::Log::warning("[DataLoader::loadFilePath] Cannot determine the type of {} - no model was loaded.", sFilePath.toStdString());
        }
    }

    endProgress();
}

//=============================================================================================================

void DataLoader::onLoadFilePressed()
{
#ifdef WASMBUILD
    auto fileContentReady = [&](const QString& sFilePath, const QByteArray& fileContent) {
        if (!sFilePath.isNull()) {
            // We need to prepend "wasm/" because QFileDialog::getOpenFileContent does not provide a full
            // path, which we need for organzing the different models in AnalyzeData
            m_pAnalyzeData->loadModel<FiffRawViewModel>("wasm/" + sFilePath, fileContent);
        }
    };
    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)", fileContentReady);
#else
    //Get the path
    QString sFilePath = QFileDialog::getOpenFileName(nullptr,
                                                     tr("Open File"),
                                                     m_sLastDir,
                                                     tr("Fiff file (*.fif *.fiff);;Event file (*.eve);;Dipole file (*.dip)"));

    QFileInfo fileInfo(sFilePath);

    if (fileInfo.fileName().isEmpty() || !fileInfo.isFile()) {
        return;
    }

    updateLastDir(fileInfo.absolutePath());

    loadFilePath(sFilePath);
#endif
}

//=============================================================================================================

void DataLoader::onLoadScanSessionPressed()
{
    QSharedPointer<FiffRawViewModel> pModel = QSharedPointer<FiffRawViewModel>::create("");
    pModel->setRealtime(true);
    m_pAnalyzeData->addModel<mne::analyze::FiffRawViewModel>(pModel, "MNE Scan Session");
}

//=============================================================================================================

void DataLoader::updateLastDir(const QString& lastDir)
{
    m_sLastDir = lastDir;
    saveSettings();
}

//=============================================================================================================

void DataLoader::saveSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
    settings.setValue("lastDirectory", m_sLastDir);
}

//=============================================================================================================

void DataLoader::loadSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
    if (settings.contains("lastDirectory")) {
        m_sLastDir = settings.value("lastDirectory").toString();
    } else {
        saveSettings();
    }
}

//=============================================================================================================

void DataLoader::onSaveFilePressed(FileType type)
{
    if (type == AVERAGE_FILE ? !m_pSelectedAverage : !m_pSelectedModel) {
        mne::Log::warning("[DataLoader::onSaveFilePressed] No model selected.");
        return;
    }

#ifdef WASMBUILD
    switch (type) {
        case DATA_FILE: {
            m_pSelectedModel->saveToFile("");
            break;
        }
        case EVENT_FILE: {
            m_pSelectedModel->getEventModel()->saveToFile("");

            break;
        }
        case AVERAGE_FILE: {
            m_pSelectedAverage->saveToFile("");
            break;
        }
        default: {
            mne::Log::warning("[DataLoader::onSaveFilePressed] Saving operation not supported.");
        }
    }
#else

    QString sFile, sFileType, sDir;

    switch (type) {
        case DATA_FILE: {
            sFile = tr("Save File");
            sFileType = tr("Fiff file(*.fif *.fiff)");
            sDir = "/../resources/data/MNE-sample-data";
            break;
        }
        case EVENT_FILE: {
            sFile = tr("Save Events");
            sFileType = tr("Event file(*.eve)");
            sDir = "/../resources/data/MNE-sample-data";
            break;
        }
        case AVERAGE_FILE: {
            sFile = tr("Save Average");
            sFileType = tr("Fiff average file(*-ave.fif)");
            sDir = "/../resources/data/MNE-sample-data";
            break;
        }
        default: {
            mne::Log::warning("[DataLoader::onSaveFilePressed] Saving operation not supported.");
        }
    }

    //Get the path
    QString sFilePath = QFileDialog::getSaveFileName(nullptr,
                                                     sFile,
                                                     QDir::currentPath() + sDir,
                                                     sFileType);
    QFileInfo fileInfo(sFilePath);

    if (fileInfo.fileName().isEmpty()) {
        return;
    }

    startProgress("Saving " + fileInfo.fileName());

    switch (type) {
        case DATA_FILE: {
            m_pSelectedModel->saveToFile(sFilePath);
            break;
        }
        case EVENT_FILE: {
            m_pSelectedModel->getEventModel()->saveToFile(sFilePath);
            break;
        }
        case AVERAGE_FILE: {
            m_pSelectedAverage->saveToFile(sFilePath);
            break;
        }
        default: {
            mne::Log::warning("[DataLoader::onSaveFilePressed] Saving operation not supported.");
        }
    }

    endProgress();

#endif
}

//=============================================================================================================

void DataLoader::onLoadSubjectPressed()
{
    QString dir = QFileDialog::getExistingDirectory(nullptr,
                                                    tr("select directory"),
                                                    QDir::currentPath() + "/../resources/data/MNE-sample-data");

    if (dir.isEmpty()) {
        mne::Log::debug("Empty input");
        return;
    }

    QDir directory = dir;

    m_pAnalyzeData->addSubject(directory.dirName());
}

//=============================================================================================================

void DataLoader::onLoadSessionPressed()
{
}

//=============================================================================================================

void DataLoader::startProgress(QString sMessage)
{
    if (!m_pProgressViewWidget->isHidden()) {
        return;
    }

    m_pProgressView->setMessage(sMessage);
    m_pProgressView->setLoadingBarVisible(false);
    m_pProgressViewWidget->show();
    m_pProgressViewWidget->move(qApp->topLevelWindows().first()->screen()->geometry().center() - m_pProgressViewWidget->rect().center());

    for (QWindow* window : qApp->topLevelWindows()) {
        window->setOpacity(0.8);
        for (QWidget* widget : window->findChildren<QWidget*>()) {
            widget->setEnabled(false);
        }
    }

    m_pProgressViewWidget->setWindowOpacity(1.0);

    QApplication::processEvents();
}

//=============================================================================================================

void DataLoader::endProgress()
{
    m_pProgressViewWidget->hide();

    for (QWindow* window : qApp->topLevelWindows()) {
        window->setOpacity(1.0);
        for (QWidget* widget : window->findChildren<QWidget*>()) {
            widget->setEnabled(true);
        }
    }
}

//=============================================================================================================

void DataLoader::onModelChanged(QSharedPointer<mne::analyze::AbstractModel> pNewModel)
{
    if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if (m_pSelectedModel) {
            if (m_pSelectedModel == pNewModel) {
                mne::Log::info("[Averaging::onModelChanged] New model is the same as old model");
                return;
            }
        }
        m_pSelectedModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
    } else if (pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_AVERAGING_MODEL) {
        m_pSelectedAverage = pNewModel;
    }
}

//=============================================================================================================

QString DataLoader::getBuildInfo()
{
    return QString(DATALOADERPLUGIN::buildDateTime()) + QString(" - ") + QString(DATALOADERPLUGIN::buildHash());
}
