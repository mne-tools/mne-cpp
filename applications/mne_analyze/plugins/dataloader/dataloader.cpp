//=============================================================================================================
/**
 * @file     dataloader.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Lorenz Esch. All rights reserved.
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

QSharedPointer<IPlugin> DataLoader::clone() const
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

    QAction* pActionSave = new QAction(tr("Save"));
    pActionLoadFile->setStatusTip(tr("Save the selected data file"));
    connect(pActionSave, &QAction::triggered,
            this, &DataLoader::onSaveFilePressed);

    QMenu* pBIDSMenu = new QMenu(tr("Load BIDS Folder"));

    pBIDSMenu->addAction(pActionLoadSubject);
    pBIDSMenu->addAction(pActionLoadSession);

    pMenuFile->addAction(pActionLoadFile);
    pMenuFile->addMenu(pBIDSMenu);
    pMenuFile->addAction(pActionSave);

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
            if(e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >()) {
                m_pSelectedModel = e->getData().value<QSharedPointer<ANSHAREDLIB::AbstractModel> >();
            }
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

    m_pProgressView->setMessage("Loading " + fileInfo.fileName());
    m_pProgressView->setLoadingBarVisible(false);
    m_pProgressViewWidget->show();
    m_pProgressViewWidget->move(qApp->topLevelWindows().first()->screen()->geometry().center() - m_pProgressViewWidget->rect().center());

    for (QWindow* window : qApp->topLevelWindows()){
        window->setOpacity(0.8);
        for (QWidget* widget : window->findChildren<QWidget*>()){
         widget->setEnabled(false);
        }
    }

    QApplication::processEvents();

    if(!fileInfo.exists() || (fileInfo.completeSuffix() != "fif")) {
        qWarning() << "[DataLoader::loadFilePath] The file does not exists or is not a .fif file.";
        return;
    }
    if(fileInfo.completeBaseName().endsWith("raw")) {
        m_pAnalyzeData->loadModel<ANSHAREDLIB::FiffRawViewModel>(sFilePath);
    } else if(fileInfo.completeBaseName().endsWith("bem")) {
        m_pAnalyzeData->loadModel<ANSHAREDLIB::BemDataModel>(sFilePath);
    } else {
        qWarning() << "[DataLoader::loadFilePath] Make sure that your files agree with the MNE naming conventions, eg.: *bem.fif; *raw.fif.";
    }

    m_pProgressViewWidget->hide();

    for (QWindow* window : qApp->topLevelWindows()){
        window->setOpacity(1.0);
        for (QWidget* widget : window->findChildren<QWidget*>()){
         widget->setEnabled(true);
        }
    }
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
                                                    tr("Fiff file(*.fif *.fiff)"));

    QFileInfo fileInfo(sFilePath);

    if(fileInfo.fileName().isEmpty()){
        return;
    }

    loadFilePath(sFilePath);
    #endif
}

//=============================================================================================================

void DataLoader::onSaveFilePressed()
{
    if(!m_pSelectedModel) {
        qWarning() << "[DataLoader::onSaveFilePressed] No model selected.";
        return;
    }

    #ifdef WASMBUILD
    m_pSelectedModel->saveToFile("");
    #else
    //Get the path
    QString sFilePath = QFileDialog::getSaveFileName(Q_NULLPTR,
                                                    tr("Save File"),
                                                    QDir::currentPath()+"/MNE-sample-data",
                                                    tr("Fiff file(*.fif *.fiff)"));

    QFileInfo fileInfo(sFilePath);

    if(fileInfo.fileName().isEmpty()){
        return;
    }

    m_pProgressView->setMessage("Saving " + fileInfo.fileName());
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

    m_pSelectedModel->saveToFile(sFilePath);

    m_pProgressViewWidget->hide();

    for (QWindow* window : qApp->topLevelWindows()){
        window->setOpacity(1.0);
        for (QWidget* widget : window->findChildren<QWidget*>()){
         widget->setEnabled(true);
        }
    }

    #endif
}

//=============================================================================================================

void DataLoader::onLoadSubjectPressed()
{
    qDebug() << "[DataLoader::onLoadFolderPressed]";

    QString dir = QFileDialog::getExistingDirectory(Q_NULLPTR,
                                                       tr("select directory"),
                                                       QDir::currentPath()+"/MNE-sample-data");

    qDebug() << "DataLoader::onLoadFolderPressed -- " << dir;

    if(dir.isEmpty()){
        qDebug() << "Empty input";
        return;
    }

    QDir directory = dir;

    qDebug() <<"Dir:" << directory.dirName() << "Items:" << directory.entryList(QDir::Dirs);

    m_pAnalyzeData->addSubject(directory.dirName());

}

//=============================================================================================================

void DataLoader::onLoadSessionPressed()
{

}
