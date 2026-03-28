//=============================================================================================================
/**
 * @file     mainwindow.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     January, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Lorenz Esch. All rights reserved.
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
 * @brief    mne_browse is the QT equivalent of the already existing C-version of mne_browse_raw. It is pursued
 *           to reimplement the full feature set of mne_browse_raw and even extend these.
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mainwindow.h"

#include <QBuffer>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MainWindow::MainWindow(QWidget *parent)
: QMainWindow(parent)
//, m_qFileRaw(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw.fif")
//, m_qEventFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis_raw-eve.fif")
//, m_qEvokedFile(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/sample_audvis-ave.fif")
, m_qSettings()
, m_rawSettings()
, ui(new Ui::MainWindowWidget)
, m_pStatusLabel(nullptr)
{
    ui->setupUi(this);

    //The following functions and their point of calling should NOT be changed
    //Setup the windows first - this NEEDS to be done here because important pointers (window pointers) which are needed for further processing are generated in this function
    setupWindowWidgets();
    setupMainWindow();

    // Setup rest of the GUI
    connectMenus();
    setWindowStatus();

    //Set standard LogLevel
    setLogLevel(_LogLvMax);
}


//*************************************************************************************************************

MainWindow::~MainWindow()
{
}


//*************************************************************************************************************

RawModel* MainWindow::rawModel() const
{
    return m_pDataWindow ? m_pDataWindow->getDataModel() : nullptr;
}


//*************************************************************************************************************

void MainWindow::setupWindowWidgets()
{
    //Create data window
    m_pDataWindow = new DataWindow(this);

    //Create dockble event window - QTDesigner used - see / FormFiles
    m_pEventWindow = new EventWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pEventWindow);
    m_pEventWindow->hide();

    //Create dockable information window - QTDesigner used - see / FormFiles
    m_pInformationWindow = new InformationWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pInformationWindow);
    m_pInformationWindow->hide();

    //Create about window - QTDesigner used - see / FormFiles
    m_pAboutWindow = new AboutWindow(this);
    m_pAboutWindow->hide();

    //Create channel info window - QTDesigner used - see / FormFiles
    m_pChInfoWindow = new ChInfoWindow(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChInfoWindow);
    m_pChInfoWindow->hide();

    //Create selection manager window - QTDesigner used - see / FormFiles
    m_pChannelSelectionViewDock = new QDockWidget(this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pChannelSelectionViewDock);
    m_pChannelSelectionView = new ChannelSelectionView(QString("MNEBrowse"),
                                                       m_pChannelSelectionViewDock,
                                                       m_pChInfoWindow->getDataModel(),
                                                       Qt::Widget);
    m_pChannelSelectionViewDock->setWidget(m_pChannelSelectionView);
    m_pChannelSelectionViewDock->hide();

    //Create average manager window - QTDesigner used - see / FormFiles
    m_pAverageWindow = new AverageWindow(this, m_qEvokedFile);
    addDockWidget(Qt::BottomDockWidgetArea, m_pAverageWindow);
    m_pAverageWindow->hide();

    //Create scale window - QTDesigner used - see / FormFiles
    m_pScaleWindow = new ScaleWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pScaleWindow);
    m_pScaleWindow->hide();

    //Create filter window - QTDesigner used - see / FormFiles
    m_pFilterWindow = new FilterWindow(this, this);
    addDockWidget(Qt::BottomDockWidgetArea, m_pFilterWindow);
    m_pFilterWindow->hide();

    //Create noise reduction window - QTDesigner used - see / FormFiles
    m_pNoiseReductionWindow = new NoiseReductionWindow(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_pNoiseReductionWindow);
    m_pNoiseReductionWindow->hide();

    //Init windows - TODO: get rid of this here, do this inside the window classes
    m_pDataWindow->init();
    m_pEventWindow->init();
    m_pScaleWindow->init();

    //Create the toolbar after all indows have been initiliased
    createToolBar();

    //Connect window signals
    //Change scaling of the data and averaged data whenever a spinbox value changed or the user performs a pinch gesture on the view
    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pDataWindow, &DataWindow::scaleData);

    connect(m_pScaleWindow, &ScaleWindow::scalingViewValueChanged,
            m_pDataWindow, &DataWindow::changeRowHeight);

    connect(m_pDataWindow, &DataWindow::scaleChannels,
            m_pScaleWindow, &ScaleWindow::scaleAllChannels);

    connect(m_pScaleWindow, &ScaleWindow::scalingChannelValueChanged,
            m_pAverageWindow, &AverageWindow::scaleAveragedData);

    //Hide non selected channels in the data view
    connect(m_pChannelSelectionView, &ChannelSelectionView::showSelectedChannelsOnly,
            m_pDataWindow, &DataWindow::showSelectedChannelsOnly);

    //Connect selection manager with average manager
    connect(m_pChannelSelectionView, &ChannelSelectionView::selectionChanged,
            m_pAverageWindow, &AverageWindow::channelSelectionManagerChanged);

    //Connect channel info window with raw data model, layout manager, average manager and the data window
    connect(rawModel(), &RawModel::fileLoaded,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::setFiffInfo);

    connect(rawModel(), &RawModel::assignedOperatorsChanged,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::assignedOperatorsChanged);

    connect(m_pChannelSelectionView, &ChannelSelectionView::loadedLayoutMap,
            m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::layoutChanged);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pChannelSelectionView, &ChannelSelectionView::setCurrentlyMappedFiffChannels);

    connect(m_pChInfoWindow->getDataModel().data(), &ChannelInfoModel::channelsMappedToLayout,
            m_pAverageWindow, &AverageWindow::setMappedChannelNames);

    //Connect selection manager with a new file loaded signal
    connect(rawModel(), &RawModel::fileLoaded,
            m_pChannelSelectionView, &ChannelSelectionView::newFiffFileLoaded);

    //Connect filter window with new file loaded signal
    connect(rawModel(), &RawModel::fileLoaded,
            m_pFilterWindow, &FilterWindow::newFileLoaded);

    //Connect noise reduction manager with fif file loading
    connect(rawModel(), &RawModel::fileLoaded,
            m_pNoiseReductionWindow, &NoiseReductionWindow::setFiffInfo);

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::projSelectionChanged,
            rawModel(), &RawModel::updateProjections);

    connect(m_pNoiseReductionWindow, &NoiseReductionWindow::compSelectionChanged,
            rawModel(), &RawModel::updateCompensator);

    //If a default file has been specified on startup -> call hideSpinBoxes and set laoded fiff channels - TODO: dirty move get rid of this here
    if(rawModel()->isFileLoaded()) {
        auto fiffInfo = rawModel()->fiffInfo();
        m_pScaleWindow->hideSpinBoxes(fiffInfo);
        m_pChInfoWindow->getDataModel()->setFiffInfo(fiffInfo);
        m_pChInfoWindow->getDataModel()->layoutChanged(m_pChannelSelectionView->getLayoutMap());
        m_pChannelSelectionView->setCurrentlyMappedFiffChannels(m_pChInfoWindow->getDataModel()->getMappedChannelsList());
        m_pChannelSelectionView->newFiffFileLoaded(fiffInfo);
        m_pFilterWindow->newFileLoaded(fiffInfo);
        m_pNoiseReductionWindow->setFiffInfo(fiffInfo);
    }
}


//*************************************************************************************************************

void MainWindow::createToolBar()
{
    //Create toolbar
    QToolBar *toolBar = new QToolBar(this);
    toolBar->setOrientation(Qt::Vertical);
    toolBar->setMovable(true);

    //Add DC removal action
    m_pRemoveDCAction = new QAction(QIcon(":/Resources/Images/removeDC.png"),tr("Remove DC component"), this);
    m_pRemoveDCAction->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
    connect(m_pRemoveDCAction,&QAction::triggered, [this](){
        if(m_pDataWindow->getDataDelegate()->isRemoveDC()) {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/removeDC.png"));
            m_pRemoveDCAction->setToolTip("Remove DC component");
            m_pDataWindow->setStatusTip(tr("Remove the DC component by subtracting the channel mean"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(false);
        }
        else {
            m_pRemoveDCAction->setIcon(QIcon(":/Resources/Images/addDC.png"));
            m_pRemoveDCAction->setToolTip("Add DC component");
            m_pRemoveDCAction->setStatusTip(tr("Add the DC component"));
            m_pDataWindow->getDataDelegate()->setRemoveDC(true);
        }

        m_pDataWindow->updateDataTableViews();
    });
    toolBar->addAction(m_pRemoveDCAction);

    //Add show/hide bad channel button
    m_pHideBadAction = new QAction(QIcon(":/Resources/Images/hideBad.png"),tr("Hide all bad channels"), this);
    m_pHideBadAction->setStatusTip(tr("Hide all bad channels"));
    connect(m_pHideBadAction,&QAction::triggered, [this](){
        if(m_pHideBadAction->toolTip() == "Show all bad channels") {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/hideBad.png"));
            m_pHideBadAction->setToolTip("Hide all bad channels");
            m_pDataWindow->setStatusTip(tr("Hide all bad channels"));
            m_pDataWindow->hideBadChannels(false);
        }
        else {
            m_pHideBadAction->setIcon(QIcon(":/Resources/Images/showBad.png"));
            m_pHideBadAction->setToolTip("Show all bad channels");
            m_pHideBadAction->setStatusTip(tr("Show all bad channels"));
            m_pDataWindow->hideBadChannels(true);
        }
    });
    toolBar->addAction(m_pHideBadAction);

    toolBar->addSeparator();

    //Toggle visibility of the event manager
    QAction* showEventManager = new QAction(QIcon(":/Resources/Images/showEventManager.png"),tr("Toggle event manager"), this);
    showEventManager->setStatusTip(tr("Toggle the event manager"));
    connect(showEventManager, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    toolBar->addAction(showEventManager);

    //Toggle visibility of the filter window
    QAction* showFilterWindow = new QAction(QIcon(":/Resources/Images/showFilterWindow.png"),tr("Toggle filter window"), this);
    showFilterWindow->setStatusTip(tr("Toggle filter window"));
    connect(showFilterWindow, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });
    toolBar->addAction(showFilterWindow);

    //Toggle visibility of the Selection manager
    QAction* showSelectionManager = new QAction(QIcon(":/Resources/Images/showSelectionManager.png"),tr("Toggle selection manager"), this);
    showSelectionManager->setStatusTip(tr("Toggle the selection manager"));
    connect(showSelectionManager, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    toolBar->addAction(showSelectionManager);

    //Toggle visibility of the scaling window
    QAction* showScalingWindow = new QAction(QIcon(":/Resources/Images/showScalingWindow.png"),tr("Toggle scaling window"), this);
    showScalingWindow->setStatusTip(tr("Toggle the scaling window"));
    connect(showScalingWindow, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    toolBar->addAction(showScalingWindow);

    //Toggle visibility of the average manager
    QAction* showAverageManager = new QAction(QIcon(":/Resources/Images/showAverageManager.png"),tr("Toggle average manager"), this);
    showAverageManager->setStatusTip(tr("Toggle the average manager"));
    connect(showAverageManager, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    toolBar->addAction(showAverageManager);

    //Toggle visibility of the noise reduction manager
    QAction* showNoiseReductionManager = new QAction(QIcon(":/Resources/Images/showNoiseReductionWindow.png"),tr("Toggle noise reduction manager"), this);
    showNoiseReductionManager->setStatusTip(tr("Toggle the noise reduction manager"));
    connect(showNoiseReductionManager, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });
    toolBar->addAction(showNoiseReductionManager);

    toolBar->addSeparator();

    //Toggle visibility of the channel information window manager
    QAction* showChInfo = new QAction(QIcon(":/Resources/Images/showChInformationWindow.png"),tr("Channel info"), this);
    showChInfo->setStatusTip(tr("Toggle channel info window"));
    connect(showChInfo, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    toolBar->addAction(showChInfo);

    //Toggle visibility of the information window
//    QAction* showInformationWindow = new QAction(QIcon(":/Resources/Images/showInformationWindow.png"),tr("Toggle information window"), this);
//    showInformationWindow->setStatusTip(tr("Toggle the information window"));
//    connect(showInformationWindow, &QAction::triggered, this, [=](){
//        showWindow(m_pInformationWindow);
//    });
//    toolBar->addAction(showInformationWindow);

    this->addToolBar(Qt::RightToolBarArea,toolBar);
}


//*************************************************************************************************************

void MainWindow::connectMenus()
{
    //File
    connect(ui->m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->m_writeAction, &QAction::triggered, this, &MainWindow::writeFile);
    connect(ui->m_loadEvents, &QAction::triggered, this, &MainWindow::loadEvents);
    connect(ui->m_saveEvents, &QAction::triggered, this, &MainWindow::saveEvents);
    connect(ui->m_loadEvokedAction, &QAction::triggered, this, &MainWindow::loadEvoked);
    connect(ui->m_quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));

    //Adjust
    connect(ui->m_filterAction, &QAction::triggered, this, [this](){
        showWindow(m_pFilterWindow);
    });

    //Windows
    connect(ui->m_eventAction, &QAction::triggered, this, [this](){
        showWindow(m_pEventWindow);
    });
    connect(ui->m_informationAction, &QAction::triggered, this, [this](){
        showWindow(m_pInformationWindow);
    });
    connect(ui->m_channelSelectionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pChannelSelectionViewDock);
    });
    connect(ui->m_averageWindowAction, &QAction::triggered, this, [this](){
        showWindow(m_pAverageWindow);
    });
    connect(ui->m_scalingAction, &QAction::triggered, this, [this](){
        showWindow(m_pScaleWindow);
    });
    connect(ui->m_ChInformationAction, &QAction::triggered, this, [this](){
        showWindow(m_pChInfoWindow);
    });
    connect(ui->m_noiseReductionManagerAction, &QAction::triggered, this, [this](){
        showWindow(m_pNoiseReductionWindow);
    });

    //Help
    connect(ui->m_aboutAction, &QAction::triggered, this, [this](){
        showWindow(m_pAboutWindow);
    });
}


//*************************************************************************************************************

void MainWindow::setupMainWindow()
{
    //set Window functions
    resize(m_qSettings.value("MainWindow/size", QSize(MAINWINDOW_WINDOW_SIZE_W, MAINWINDOW_WINDOW_SIZE_H)).toSize()); //Resize to predefined default size
    move(m_qSettings.value("MainWindow/position", QPoint(MAINWINDOW_WINDOW_POSITION_X, MAINWINDOW_WINDOW_POSITION_Y)).toPoint()); // Move this main window to position 50/50 on the screen

    //Set data window as central widget - This is needed because we are using QDockWidgets
    setCentralWidget(m_pDataWindow);
}


//*************************************************************************************************************

void MainWindow::setWindowStatus()
{
    //Set window title
    QString title;
    //title = QString("%1").arg(CInfo::AppNameShort());
    title = QString("Visualize and Process");
    setWindowTitle(title);

    //Set status bar
    //Set data file informations
    if (m_pDataWindow->isFiffFileLoaded()) {
        double dur = m_pDataWindow->fiffFileDurationSeconds();
        int durMin = static_cast<int>(dur) / 60;
        double durSec = dur - durMin * 60.0;
        QString durStr = durMin > 0
            ? QString("%1 min %2 s").arg(durMin).arg(durSec, 0, 'f', 1)
            : QString("%1 s").arg(dur, 0, 'f', 1);
        title = QString("Data file: %1  |  Duration: %2")
            .arg(m_pDataWindow->fiffFileName(), durStr);
    } else if (rawModel()->isFileLoaded() && rawModel()->fiffInfo()) {
        int idx = m_qFileRaw.fileName().lastIndexOf("/");
        QString filename = m_qFileRaw.fileName().remove(0,idx+1);
        title = QString("Data file: %1  /  First sample: %2  /  Sample frequency: %3Hz").arg(filename).arg(rawModel()->firstSample()).arg(rawModel()->fiffInfo()->sfreq);
    } else {
        title = QString("No data file");
    }

    //Set event file informations
    if(m_pEventWindow->getEventModel()->isFileLoaded()) {
        int idx = m_qEventFile.fileName().lastIndexOf("/");
        QString filename = m_qEventFile.fileName().remove(0,idx+1);

        title.append(QString("  -  Event file: %1").arg(filename));
    }
    else
        title.append("  -  No event file");

    //Set evoked file informations
    if(m_pAverageWindow->getAverageModel()->isFileLoaded()) {
        int idx = m_qEvokedFile.fileName().lastIndexOf("/");
        QString filename = m_qEvokedFile.fileName().remove(0,idx+1);

        title.append(QString("  -  Evoked file: %1").arg(filename));
    }
    else
        title.append("  -  No evoked file");

    if(!m_pStatusLabel) {
        m_pStatusLabel = new QLabel(this);
        statusBar()->addWidget(m_pStatusLabel, 1);
    }
    m_pStatusLabel->setText(title);
}


//*************************************************************************************************************
//Log
void MainWindow::writeToLog(const QString& logMsg, LogKind lgknd, LogLevel lglvl)
{
    m_pInformationWindow->writeToLog(logMsg, lgknd, lglvl);
}


//*************************************************************************************************************

void MainWindow::setLogLevel(LogLevel lvl)
{
    m_pInformationWindow->setLogLevel(lvl);
}


//*************************************************************************************************************
// SLOTS
#ifdef WASMBUILD
static QByteArray s_wasmByteArray;
#endif

void MainWindow::openFile()
{
#ifdef WASMBUILD
    auto fileContentReady = [&](const QString &fileName, const QByteArray &fileContent) {
        if (!fileName.isEmpty()) {
            m_qFileRaw.setFileName(fileName);

            //Clear event model
            m_pEventWindow->getEventModel()->clearModel();

            s_wasmByteArray = fileContent;
            QBuffer* buffer = new QBuffer(&s_wasmByteArray);
            if(rawModel()->loadFiffData(buffer))
                qInfo() << "Fiff data file" << fileName << "loaded.";
            else
                qWarning() << "ERROR loading fiff data file" << fileName;

            //set position of QScrollArea
            m_pDataWindow->getDataTableView()->horizontalScrollBar()->setValue(0);
            m_pDataWindow->initMVCSettings();

            //Set fiffInfo in event model
            m_pEventWindow->getEventModel()->setFiffInfo(rawModel()->fiffInfo());
            m_pEventWindow->getEventModel()->setFirstLastSample(rawModel()->firstSample(),
                                                                rawModel()->lastSample());

            //resize columns to contents - needs to be done because the new data file can be shorter than the old one
            m_pDataWindow->updateDataTableViews();
            m_pDataWindow->getDataTableView()->resizeColumnsToContents();

            //Update status bar
            setWindowStatus();

            //Hide not presented channel types and their spin boxes in the scale window
            m_pScaleWindow->hideSpinBoxes(rawModel()->fiffInfo());

            m_qFileRaw.close();
        }
    };
    QFileDialog::getOpenFileContent("Fiff File (*.fif *.fiff)",  fileContentReady);
#else
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadRawFile(filename);
#endif
}


//*************************************************************************************************************

void MainWindow::writeFile()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Write fiff data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    if(filename == m_qFileRaw.fileName()) {
        QMessageBox msgBox;
        msgBox.setText("You are trying to write to the file you are currently loading the data from. Please choose another file to write to.");
        msgBox.exec();
        return;
    }

    //Create output file, progress dialog and future watcher
    QFile qFileOutput (filename);
    if(qFileOutput.isOpen())
        qFileOutput.close();

    QFutureWatcher<bool> writeFileFutureWatcher;
    QProgressDialog progressDialog("Writing to fif file...", QString(), 0, 0, this, Qt::Dialog);

    //Connect future watcher and dialog
    connect(&writeFileFutureWatcher, &QFutureWatcher<bool>::finished,
            &progressDialog, &QProgressDialog::reset);

    connect(&progressDialog, &QProgressDialog::canceled,
            &writeFileFutureWatcher, &QFutureWatcher<bool>::cancel);

    connect(rawModel(), &RawModel::writeProgressRangeChanged,
            &progressDialog, &QProgressDialog::setRange);

    connect(rawModel(), &RawModel::writeProgressChanged,
            &progressDialog, &QProgressDialog::setValue);

    //Run the file writing in seperate thread
    writeFileFutureWatcher.setFuture(QtConcurrent::run([this, &qFileOutput](){
        return rawModel()->writeFiffData(&qFileOutput);
    }));

    progressDialog.exec();

    writeFileFutureWatcher.waitForFinished();

    if(!writeFileFutureWatcher.future().result())
        qWarning() << "MainWindow: ERROR writing fiff data file" << qFileOutput.fileName();
    else
        qInfo() << "MainWindow: Successfully written to" << qFileOutput.fileName();
}


//*************************************************************************************************************

void MainWindow::loadEvents()
{
    QString filename = QFileDialog::getOpenFileName(this,
                                                    QString("Open fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    loadEventsFile(filename);
}


//*************************************************************************************************************

void MainWindow::saveEvents()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    QString("Save fiff event data file"),
                                                    QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),
                                                    tr("fif event data files (*-eve.fif);;fif data files (*.fif)"));
    if(filename.isEmpty())
        return;

    if(m_qEventFile.isOpen())
        m_qEventFile.close();
    m_qEventFile.setFileName(filename);

    if(m_pEventWindow->getEventModel()->saveEventData(m_qEventFile))
        qInfo() << "Fiff event data file" << filename << "saved.";
    else
        qWarning() << "ERROR saving fiff event data file" << filename;
}


//*************************************************************************************************************

bool MainWindow::loadRawFile(const QString& filename)
{
    if(m_qFileRaw.isOpen())
        m_qFileRaw.close();

    m_qFileRaw.setFileName(filename);

    m_pEventWindow->getEventModel()->clearModel();

    // ── ChannelDataView path: demand-paged, opens header only (fast) ──
    const bool ok = m_pDataWindow->loadFiffFile(filename);
    if(ok)
        qInfo() << "Fiff data file" << filename << "loaded (ChannelDataView).";
    else
        qWarning() << "ERROR loading fiff data file" << filename;

    // ── Legacy RawModel path: kept for filter/event sub-systems ──────
    // Runs in a background thread so the ChannelDataView path (above) can
    // display data immediately while the full FIFF index is parsed.
    // Wait for any previous in-flight load to finish before starting a new one
    // to avoid concurrent access to RawModel internals.
    if (m_legacyLoadWatcher.isRunning())
        m_legacyLoadWatcher.waitForFinished();

    // Disconnect any stale finished connection from a prior load
    disconnect(&m_legacyLoadWatcher, &QFutureWatcher<bool>::finished, nullptr, nullptr);

    connect(&m_legacyLoadWatcher, &QFutureWatcher<bool>::finished, this, [this]() {
        auto fiffInfo = rawModel()->fiffInfo();
        m_pEventWindow->getEventModel()->setFiffInfo(fiffInfo);
        m_pEventWindow->getEventModel()->setFirstLastSample(rawModel()->firstSample(),
                                                            rawModel()->lastSample());
        setWindowStatus();
        if (fiffInfo)
            m_pScaleWindow->hideSpinBoxes(fiffInfo);
        m_qFileRaw.close();
    });

    m_legacyLoadWatcher.setFuture(
        QtConcurrent::run([this]() -> bool {
            return rawModel()->loadFiffData(&m_qFileRaw);
        })
    );

    setWindowStatus();
    return ok;
}


//*************************************************************************************************************

bool MainWindow::loadEventsFile(const QString& filename)
{
    if(m_qEventFile.isOpen())
        m_qEventFile.close();

    m_qEventFile.setFileName(filename);

    const bool ok = m_pEventWindow->getEventModel()->loadEventData(m_qEventFile);
    if(ok)
        qInfo() << "Fiff event data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading fiff event data file" << filename;

    setWindowStatus();

    if(ok && !m_pEventWindow->isVisible())
        m_pEventWindow->show();

    return ok;
}


//*************************************************************************************************************

void MainWindow::applyCommandLineOptions(const QString& rawFile,
                                         const QString& eventsFile,
                                         double highpass,
                                         double lowpass)
{
    if(!rawFile.isEmpty()) {
        if(!QFile::exists(rawFile)) {
            qWarning() << "[mne_browse] --raw: file not found:" << rawFile;
        } else {
            loadRawFile(rawFile);
        }
    }

    if(!eventsFile.isEmpty()) {
        if(!QFile::exists(eventsFile)) {
            qWarning() << "[mne_browse] --events: file not found:" << eventsFile;
        } else {
            loadEventsFile(eventsFile);
        }
    }

    if(highpass >= 0.0 || lowpass >= 0.0) {
        m_pFilterWindow->setFrequencies(highpass, lowpass);
        m_pFilterWindow->applyFilter();
        if(!m_pFilterWindow->isVisible())
            m_pFilterWindow->show();
    }
}


//*************************************************************************************************************

void MainWindow::loadEvoked()
{
    QString filename = QFileDialog::getOpenFileName(this,QString("Open evoked fiff data file"),QString(QCoreApplication::applicationDirPath() + "/MNE-sample-data/MEG/sample/"),tr("fif evoked data files (*-ave.fif);;fif data files (*.fif)"));

    if(filename.isEmpty())
        return;

    if(m_qEvokedFile.isOpen())
        m_qEvokedFile.close();

    m_qEvokedFile.setFileName(filename);

    if(m_pAverageWindow->getAverageModel()->loadEvokedData(m_qEvokedFile))
        qInfo() << "Fiff evoked data file" << filename << "loaded.";
    else
        qWarning() << "ERROR loading evoked data file" << filename;

    //Update status bar
    setWindowStatus();

    //Show average window
//    if(!m_pAverageWindow->isVisible())
//        m_pAverageWindow->show();
}


//*************************************************************************************************************

void MainWindow::showWindow(QWidget *window)
{
    //Note: A widget that happens to be obscured by other windows on the screen is considered to be visible.
    if(!window->isVisible())
    {
        window->show();
        window->raise();
    }
    else // if visible raise the widget to be sure that it is not obscured by other windows
        window->hide();
}
