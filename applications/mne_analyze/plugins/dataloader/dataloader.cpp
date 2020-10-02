//=============================================================================================================
/**
 * @file     dataloader.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch, Gabriel Motta. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Definition of the DataLoader class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dataloader.h"

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/bemdatamodel.h>
#include <anShared/Model/annotationmodel.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/progressview.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DATALOADERPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DataLoader::DataLoader()
: m_pProgressView(new DISPLIB::ProgressView(false))
, m_pProgressViewWidget(new QWidget())
{
    m_pProgressViewWidget->setWindowFlags(Qt::Window);

    QVBoxLayout* layout = new QVBoxLayout(m_pProgressViewWidget.data());
    layout->addWidget(m_pProgressView);
    m_pProgressViewWidget->setLayout(layout);
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

QMenu *DataLoader::getMenu()
{
    QMenu* pMenuFile = new QMenu(tr("File"));

    QAction* pActionLoadFile = new QAction(tr("Open File"));
    pActionLoadFile->setStatusTip(tr("Load a data file"));
    connect(pActionLoadFile, &QAction::triggered,
            this, &DataLoader::onLoadFilePressed);

    QAction* pActionLoadSubject = new QAction(tr("Open Subject"));
    pActionLoadSubject->setStatusTip(tr("Load a subject folder"));
    connect(pActionLoadSubject, &QAction::triggered,
            this, &DataLoader::onLoadSubjectPressed);

    QAction* pActionLoadSession = new QAction(tr("Open Session"));
    pActionLoadSession->setStatusTip(tr("Load a session folder"));
    connect(pActionLoadSession, &QAction::triggered,
            this, &DataLoader::onLoadSessionPressed);

    QAction* pActionSaveData = new QAction(tr("Save data"));
    pActionLoadFile->setStatusTip(tr("Save the selected data file"));
    connect(pActionSaveData, &QAction::triggered,[=] {
                onSaveFilePressed(DATA_FILE);
            });

    QAction* pActionSaveAvg = new QAction(tr("Save average"));
    pActionLoadFile->setStatusTip(tr("Save the selected data file"));
    connect(pActionSaveAvg, &QAction::triggered,[=] {
                onSaveFilePressed(AVERAGE_FILE);
            });

    QAction* pActionSaveAnn = new QAction(tr("Save annotation"));
    pActionLoadFile->setStatusTip(tr("Save the selected data file"));
    connect(pActionSaveAnn, &QAction::triggered,[=] {
                onSaveFilePressed(ANNOTATION_FILE);
            });

    QMenu* pBIDSMenu = new QMenu(tr("Load BIDS Folder"));
    pBIDSMenu->addAction(pActionLoadSubject);
    pBIDSMenu->addAction(pActionLoadSession);

    QMenu* pSaveMenu = new QMenu(tr("Save"));
    pSaveMenu->addAction(pActionSaveData);
    pSaveMenu->addAction(pActionSaveAnn);
    //pSaveMenu->addAction(pActionSaveAvg);

    pMenuFile->addAction(pActionLoadFile);
    //pMenuFile->addMenu(pBIDSMenu);
    pMenuFile->addMenu(pSaveMenu);
    //pMenuFile->addAction(pActionSave);

    return pMenuFile;
}

//=============================================================================================================

QDockWidget *DataLoader::getControl()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *DataLoader::getView()
{
    return Q_NULLPTR;
}

//=============================================================================================================

void DataLoader::handleEvent(QSharedPointer<Event> e)
{
    switch (e->getType()) {
    case EVENT_TYPE::SELECTED_MODEL_CHANGED:
        onModelChanged(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >());
        break;
    default:
        qWarning() << "[DataLoader::handleEvent] Received an Event that is not handled by switch cases.";
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
    if(sArguments.size() == 2 && (sArguments.first() == "file" || sArguments.first() == "f")) {
        loadFilePath(sArguments.at(1));
    }
}

//=============================================================================================================

void DataLoader::loadFilePath(const QString& sFilePath)
{
    QFileInfo fileInfo(sFilePath);

    startProgress("Loading " + fileInfo.fileName());

    if(fileInfo.exists() && (fileInfo.completeSuffix() == "eve")){
        QSharedPointer<ANSHAREDLIB::AnnotationModel> pModel = m_pAnalyzeData->loadModel<ANSHAREDLIB::AnnotationModel>(sFilePath);
        //pModel->applyOffset(m_pSelectedModel->absoluteFirstSample());
        pModel->setFiffModel(m_pSelectedModel);
        pModel->setFirstLastSample(m_pSelectedModel->absoluteFirstSample(), m_pSelectedModel->absoluteLastSample());
        pModel->setSampleFreq(m_pSelectedModel->getFiffInfo()->sfreq);
    } else if(fileInfo.exists() && (fileInfo.completeSuffix() == "fif")) {
        if(fileInfo.completeBaseName().endsWith("eve")){
            m_pAnalyzeData->loadModel<ANSHAREDLIB::AnnotationModel>(sFilePath);
        } else if(fileInfo.completeBaseName().endsWith("bem")) {
            m_pAnalyzeData->loadModel<ANSHAREDLIB::BemDataModel>(sFilePath);
        } else if(fileInfo.completeBaseName().endsWith("raw")){
            m_pAnalyzeData->loadModel<ANSHAREDLIB::FiffRawViewModel>(sFilePath);
        } else if (fileInfo.completeBaseName().endsWith("ave")){
            m_pAnalyzeData->loadModel<ANSHAREDLIB::AveragingDataModel>(sFilePath);
        }
    }

    endProgress();
}

//=============================================================================================================

void DataLoader::onLoadFilePressed()
{
    #ifdef WASMBUILD
    auto fileContentReady = [&](const QString &sFilePath, const QByteArray &fileContent) {
        if(!sFilePath.isNull()) {
            // We need to prepend "wasm/" because QFileDialog::getOpenFileContent does not provide a full
            // path, which we need for organzing the different models in AnalyzeData
            m_pAnalyzeData->loadModel<FiffRawViewModel>("wasm/"+sFilePath, fileContent);
        }
    };
    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)",  fileContentReady);
    #else
    //Get the path
    QString sFilePath = QFileDialog::getOpenFileName(Q_NULLPTR,
                                                    tr("Open File"),
                                                    QDir::currentPath()+"/MNE-sample-data",
                                                    tr("Fiff file (*.fif *.fiff);;Event file (*.eve)"));

    QFileInfo fileInfo(sFilePath);

    if(fileInfo.fileName().isEmpty()){
        return;
    }

    loadFilePath(sFilePath);
    #endif
}

//=============================================================================================================

void DataLoader::onSaveFilePressed(FileType type)
{
    if(!m_pSelectedModel) {
        qWarning() << "[DataLoader::onSaveFilePressed] No model selected.";
        return;
    }

    #ifdef WASMBUILD
    switch (type){
        case DATA_FILE:{
            m_pSelectedModel->saveToFile("");
            break;
        }
        case ANNOTATION_FILE: {
            m_pSelectedModel->getAnnotationModel()->saveToFile("");

            break;
        }
        case AVERAGE_FILE: {
            qDebug() << "[DataLoader::onSaveFilePressed] AVERAGE_FILE Not yet implemented";
            break;
        }
        default: {
            qWarning() << "[DataLoader::onSaveFilePressed] Saving operation not supported.";
        }
    }
    #else

    QString sFile, sFileType, sDir;

    switch (type){
        case DATA_FILE:{
            sFile = tr("Save File");
            sFileType = tr("Fiff file(*.fif *.fiff)");
            sDir = "/MNE-sample-data";
            break;
        }
        case ANNOTATION_FILE: {
            sFile = tr("Save Events");
            sFileType = tr("Event file(*.eve)");
            sDir = "/MNE-sample-data";
            break;
        }
        case AVERAGE_FILE: {
            qDebug() << "[DataLoader::onSaveFilePressed] Not yet implemented";
            return;
            break;
        }
        default: {
            qWarning() << "[DataLoader::onSaveFilePressed] Saving operation not supported.";
        }
    }

    //Get the path
    QString sFilePath = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                    sFile,
                                                    QDir::currentPath()+sDir,
                                                    sFileType);
    QFileInfo fileInfo(sFilePath);

    if(fileInfo.fileName().isEmpty()){
        return;
    }

    startProgress("Saving " + fileInfo.fileName());

    switch (type){
        case DATA_FILE:{
            m_pSelectedModel->saveToFile(sFilePath);
            break;
        }
        case ANNOTATION_FILE: {
            m_pSelectedModel->getAnnotationModel()->saveToFile(sFilePath);
            break;
        }
        case AVERAGE_FILE: {
            qDebug() << "[DataLoader::onSaveFilePressed] AVERAGE_FILE Not yet implemented";
            break;
        }
        default: {
            qWarning() << "[DataLoader::onSaveFilePressed] Saving operation not supported.";
        }
    }

    endProgress();

    #endif
}

//=============================================================================================================

void DataLoader::onLoadSubjectPressed()
{
    QString dir = QFileDialog::getExistingDirectory(Q_NULLPTR,
                                                       tr("select directory"),
                                                       QDir::currentPath()+"/MNE-sample-data");

    if(dir.isEmpty()){
        qDebug() << "Empty input";
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
    if (!m_pProgressViewWidget->isHidden()){
        return;
    }

    m_pProgressView->setMessage(sMessage);
    m_pProgressView->setLoadingBarVisible(false);
    m_pProgressViewWidget->show();
    m_pProgressViewWidget->move(qApp->topLevelWindows().first()->screen()->geometry().center() - m_pProgressViewWidget->rect().center());

    for (QWindow* window : qApp->topLevelWindows()){
        window->setOpacity(0.8);
        for (QWidget* widget : window->findChildren<QWidget*>()){
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

    for (QWindow* window : qApp->topLevelWindows()){
        window->setOpacity(1.0);
        for (QWidget* widget : window->findChildren<QWidget*>()){
         widget->setEnabled(true);
        }
    }
}

//=============================================================================================================

void DataLoader::onModelChanged(QSharedPointer<ANSHAREDLIB::AbstractModel> pNewModel)
{
    if(pNewModel->getType() == MODEL_TYPE::ANSHAREDLIB_FIFFRAW_MODEL) {
        if(m_pSelectedModel) {
            if(m_pSelectedModel == pNewModel) {
                qInfo() << "[Averaging::onModelChanged] New model is the same as old model";
                return;
            }
        }
        m_pSelectedModel = qSharedPointerCast<FiffRawViewModel>(pNewModel);
    }
}
