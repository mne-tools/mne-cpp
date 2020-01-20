//=============================================================================================================
/**
 * @file     neuromag.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the Neuromag class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "neuromag.h"
#include "neuromagproducer.h"

#include "FormFiles/neuromagsetupwidget.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>
#include <scMeas/realtimemultisamplearray.h>
#include <disp/viewers/projectsettingsview.h>
#include <fiff/fiff_info.h>
#include <utils/ioutils.h>
#include <communication/rtClient/rtcmdclient.h>
#include <disp3D/viewers/hpiview.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDir>
#include <QSettings>
#include <QMessageBox>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NEUROMAGPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace IOBUFFER;
using namespace SCMEASLIB;
using namespace FIFFLIB;
using namespace DISPLIB;
using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Neuromag::Neuromag()
: m_sNeuromagClientAlias("mne_scan")
, m_pRtCmdClient(NULL)
, m_bCmdClientIsConnected(false)
, m_sNeuromagIP("172.21.16.88")//("127.0.0.1")
, m_pNeuromagProducer(new NeuromagProducer(this))
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(0)
, m_bIsRunning(false)
, m_sFiffHeader(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/neuromag/header.fif")
, m_iActiveConnectorId(0)
, m_bWriteToFile(false)
, m_iRecordingMSeconds(5*60*1000)
, m_iBlinkStatus(-1)
, m_iSplitCount(0)
, m_bUseRecordTimer(false)
{
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup Project"),this);
    m_pActionSetupProject->setStatusTip(tr("Setup Project"));
    connect(m_pActionSetupProject.data(), &QAction::triggered,
            this, &Neuromag::showProjectDialog);
    addPluginAction(m_pActionSetupProject);

    m_pActionRecordFile = new QAction(QIcon(":/images/record.png"), tr("Start Recording"),this);
    m_pActionRecordFile->setStatusTip(tr("Start Recording"));
    connect(m_pActionRecordFile.data(), &QAction::triggered,
            this, &Neuromag::toggleRecordingFile);
    addPluginAction(m_pActionRecordFile);

    //Init HPI
    m_pActionComputeHPI = new QAction(QIcon(":/images/latestFiffInfoHPI.png"), tr("Compute HPI"),this);
    m_pActionComputeHPI->setStatusTip(tr("Compute HPI"));
    connect(m_pActionComputeHPI.data(), &QAction::triggered,
            this, &Neuromag::showHPIDialog);
    addPluginAction(m_pActionComputeHPI);

    //Init recording timers
    m_pRecordTimer = QSharedPointer<QTimer>(new QTimer(this));
    m_pRecordTimer->setSingleShot(true);
    connect(m_pRecordTimer.data(), &QTimer::timeout,
            this, &Neuromag::toggleRecordingFile);

    m_pBlinkingRecordButtonTimer = QSharedPointer<QTimer>(new QTimer(this));
    connect(m_pBlinkingRecordButtonTimer.data(), &QTimer::timeout,
            this, &Neuromag::changeRecordingButton);

    m_pUpdateTimeInfoTimer = QSharedPointer<QTimer>(new QTimer(this));
    connect(m_pUpdateTimeInfoTimer.data(), &QTimer::timeout,
            this, &Neuromag::onRecordingRemainingTimeChange);
}


//*************************************************************************************************************

Neuromag::~Neuromag()
{
    if(m_pNeuromagProducer->isRunning() || this->isRunning())
        stop();
}


//*************************************************************************************************************

void Neuromag::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

QSharedPointer<SCSHAREDLIB::IPlugin> Neuromag::clone() const
{
    QSharedPointer<Neuromag> pNeuromagClone(new Neuromag());
    return pNeuromagClone;
}


//*************************************************************************************************************

void Neuromag::init()
{
    //Neuromag data Path
    QString sNeuromagDataPath = QDir::homePath() + "/NeuromagData";
    if(!QDir(sNeuromagDataPath).exists()) {
        QDir().mkdir(sNeuromagDataPath);
    }

    //Test Project
    QSettings settings;
    QString sCurrentProject = settings.value(QString("Plugin/%1/currentProject").arg(getName()), "TestProject").toString();
    if(!QDir(sNeuromagDataPath+"/"+sCurrentProject).exists()) {
        QDir().mkdir(sNeuromagDataPath+"/"+sCurrentProject);
    }

    //Test Subject
    QString sCurrentSubject = settings.value(QString("Plugin/%1/currentSubject").arg(getName()), "TestSubject").toString();
    if(!QDir(sNeuromagDataPath+"/"+sCurrentProject+"/"+sCurrentSubject).exists()) {
        QDir().mkdir(sNeuromagDataPath+"/"+sCurrentProject+"/"+sCurrentSubject);
    }

    m_pProjectSettingsView = QSharedPointer<ProjectSettingsView>::create(sNeuromagDataPath,
                                                                         sCurrentProject,
                                                                         sCurrentSubject,
                                                                         "");
    m_sRecordFile = m_pProjectSettingsView->getCurrentFileName();

    connect(m_pProjectSettingsView.data(), &ProjectSettingsView::timerChanged,
            this, &Neuromag::setRecordingTimerChanged);

    connect(m_pProjectSettingsView.data(), &ProjectSettingsView::recordingTimerStateChanged,
            this, &Neuromag::setRecordingTimerStateChanged);

    m_pProjectSettingsView->hide();

    // Start NeuromagProducer
    m_pNeuromagProducer->start();

    //init channels when fiff info is available
    connect(this, &Neuromag::fiffInfoAvailable,
            this, &Neuromag::initConnector);

    //Try to connect the cmd client on start up using localhost connection
    this->connectCmdClient();
}


//*************************************************************************************************************

void Neuromag::unload()
{

}


//*************************************************************************************************************

void Neuromag::showProjectDialog()
{
    if(m_pProjectSettingsView) {
        m_pProjectSettingsView->show();
    }
}


//*************************************************************************************************************

void Neuromag::splitRecordingFile()
{
    qDebug() << "Neuromag::splitRecordingFile - Splitting recording file";
    ++m_iSplitCount;
    QString nextFileName = m_sRecordFile.remove("_raw.fif");
    nextFileName += QString("-%1_raw.fif").arg(m_iSplitCount);

    // Write the link to the next file
    qint32 data;
    m_pOutfid->start_block(FIFFB_REF);
    data = FIFFV_ROLE_NEXT_FILE;
    m_pOutfid->write_int(FIFF_REF_ROLE,&data);
    m_pOutfid->write_string(FIFF_REF_FILE_NAME, nextFileName);
    m_pOutfid->write_id(FIFF_REF_FILE_ID);//ToDo meas_id
    data = m_iSplitCount - 1;
    m_pOutfid->write_int(FIFF_REF_FILE_NUM, &data);
    m_pOutfid->end_block(FIFFB_REF);

    //finish file
    m_pOutfid->finish_writing_raw();

    //start next file
    m_qFileOut.setFileName(nextFileName);
    RowVectorXd cals;
    m_pOutfid = FiffStream::start_writing_raw(m_qFileOut,
                                              *m_pFiffInfo,
                                              cals);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
}


//*************************************************************************************************************

void Neuromag::toggleRecordingFile()
{
    //Setup writing to file
    if(m_bWriteToFile) {
        m_mutex.lock();
        m_pOutfid->finish_writing_raw();
        m_mutex.unlock();

        m_bWriteToFile = false;
        m_iSplitCount = 0;

        //Stop record timer
        m_pRecordTimer->stop();
        m_pUpdateTimeInfoTimer->stop();
        m_pBlinkingRecordButtonTimer->stop();

        m_pActionRecordFile->setIcon(QIcon(":/images/record.png"));
    } else {
        m_iSplitCount = 0;

        if(!m_pFiffInfo) {
            QMessageBox msgBox;
            msgBox.setText("FiffInfo missing!");
            msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
            msgBox.exec();
            return;
        }

        if(m_pFiffInfo->dev_head_t.trans.isIdentity()) {
            QMessageBox msgBox;
            msgBox.setText("It seems that no HPI fitting was performed. This is your last chance!");
            msgBox.setInformativeText(" Do you want to continue without HPI fitting?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
            int ret = msgBox.exec();
            if(ret == QMessageBox::No)
                return;
        }

        //Initiate the stream for writing to the fif file
        if(m_pProjectSettingsView) {
            m_sRecordFile = m_pProjectSettingsView->getCurrentFileName();
        }

        m_qFileOut.setFileName(m_sRecordFile);
        if(m_qFileOut.exists()) {
            QMessageBox msgBox;
            msgBox.setText("The file you want to write already exists.");
            msgBox.setInformativeText("Do you want to overwrite this file?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setWindowFlags(Qt::WindowStaysOnTopHint);
            int ret = msgBox.exec();
            if(ret == QMessageBox::No)
                return;
        }

        //Set all projectors to zero before writing to file because we always write the raw data
        for(int i = 0; i<m_pFiffInfo->projs.size(); i++) {
            m_pFiffInfo->projs[i].active = false;
        }

        m_mutex.lock();
        RowVectorXd cals;
        m_pOutfid = FiffStream::start_writing_raw(m_qFileOut,
                                                  *m_pFiffInfo,
                                                  cals);
        fiff_int_t first = 0;
        m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
        m_mutex.unlock();

        m_bWriteToFile = true;

        //Start timers for record button blinking, recording timer and updating the elapsed time in the proj widget
        m_pBlinkingRecordButtonTimer->start(500);
        m_recordingStartedTime.restart();
        m_pUpdateTimeInfoTimer->start(1000);

        if(m_bUseRecordTimer) {
            m_pRecordTimer->start(m_iRecordingMSeconds);
        }
    }
}


//*************************************************************************************************************

void Neuromag::setRecordingTimerChanged(int timeMSecs)
{
    //If the recording time is changed during the recording, change the timer
    if(m_bWriteToFile) {
        m_pRecordTimer->setInterval(timeMSecs-m_recordingStartedTime.elapsed());
    }

    m_iRecordingMSeconds = timeMSecs;
}


//*************************************************************************************************************

void Neuromag::setRecordingTimerStateChanged(bool state)
{
    m_bUseRecordTimer = state;
}


//*************************************************************************************************************

void Neuromag::changeConnector(qint32 p_iNewConnectorId)
{
    if(p_iNewConnectorId != m_iActiveConnectorId)
    {
        // read meas info
        (*m_pRtCmdClient)["selcon"].pValues()[0].setValue(p_iNewConnectorId);
        (*m_pRtCmdClient)["selcon"].send();

        m_iActiveConnectorId = p_iNewConnectorId;

        // clear all and request everything new
        clear();

        //
        // request available commands
        //
        m_pRtCmdClient->requestCommands();

        //
        // Read Info
        //
        if(!m_pFiffInfo)
            requestInfo();

        //
        // Read Buffer Size
        //
        m_iBufferSize = m_pRtCmdClient->requestBufsize();

        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

bool Neuromag::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning()) {
//        QThread::wait();
//    }

    if(m_bCmdClientIsConnected && m_pFiffInfo) {
        // Initialize real time measurements
        init();

        //Set buffer size
        (*m_pRtCmdClient)["bufsize"].pValues()[0].setValue(m_iBufferSize);
        (*m_pRtCmdClient)["bufsize"].send();

        // Buffer
        m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8,m_pFiffInfo->nchan,m_iBufferSize));

        m_bIsRunning = true;

        // Start threads
        QThread::start();

        m_pNeuromagProducer->start();

        while(!m_pNeuromagProducer->m_bFlagMeasuring)
            msleep(1);

        // Start Measurement at rt_Server
        // start measurement
        (*m_pRtCmdClient)["start"].pValues()[0].setValue(m_pNeuromagProducer->m_iDataClientId);
        (*m_pRtCmdClient)["start"].send();

        return true;
    } else {
        if(!m_pFiffInfo) {
            qWarning()<<"Neuromag::start - FiffInfo is empty (NULL).";
        }
        if(!m_bCmdClientIsConnected) {
            qWarning()<<"Neuromag::start - m_bCmdClientIsConnected is false.";
        }
        return false;
    }
}


//*************************************************************************************************************

bool Neuromag::stop()
{
    if(m_pNeuromagProducer->isRunning()) {
        m_pNeuromagProducer->stop();
    }

    //Wait until this thread (TMSI) is stopped
    m_bIsRunning = false;

    if(this->isRunning())  {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pRawMatrixBuffer_In->releaseFromPop();

        m_pRawMatrixBuffer_In->clear();

        m_pRTMSA_Neuromag->data()->clear();
    }

    if(m_pHPIWidget) {
        m_pHPIWidget->hide();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Neuromag::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString Neuromag::getName() const
{
    return "Neuromag";
}


//*************************************************************************************************************

QWidget* Neuromag::setupWidget()
{
    NeuromagSetupWidget* widget = new NeuromagSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    return widget;
}


//*************************************************************************************************************

bool Neuromag::readProjectors()
{
    QFile t_headerFiffFile(m_sFiffHeader);

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&t_headerFiffFile));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()) {
        return false;
    }

    QList<FiffProj> q_ListProj = t_pStream->read_proj(t_pStream->dirtree());

    if (q_ListProj.size() == 0) {
        printf("Could not find projectors\n");
        return false;
    }

    m_pFiffInfo->projs = q_ListProj;

    //
    //   Activate the projection items
    //
    for (qint32 k = 0; k < m_pFiffInfo->projs.size(); ++k) {
        m_pFiffInfo->projs[k].active = true;
    }

    //garbage collecting
    t_pStream->close();

    return true;
}


//*************************************************************************************************************

void Neuromag::connectCmdClient()
{
    if(m_pRtCmdClient.isNull())
        m_pRtCmdClient = QSharedPointer<RtCmdClient>(new RtCmdClient);
    else if(m_bCmdClientIsConnected)
        this->disconnectCmdClient();

    m_pRtCmdClient->connectToHost(m_sNeuromagIP);
    m_pRtCmdClient->waitForConnected(1000);

    if(m_pRtCmdClient->state() == QTcpSocket::ConnectedState)
    {
        m_mutex.lock();

        if(!m_bCmdClientIsConnected)
        {
            //
            // request available commands
            //
            m_pRtCmdClient->requestCommands();

            //
            // set cmd client is connected
            //
            m_bCmdClientIsConnected = true;

            //
            // Read Info
            //
            if(!m_pFiffInfo)
                requestInfo();

            // This will read projectors from an external file and replace the one received from mne_rt_server
            //if(m_pFiffInfo)
            //    readProjectors();

            //
            // Read Connectors
            //
            if(m_qMapConnectors.size() == 0)
                m_iActiveConnectorId = m_pRtCmdClient->requestConnectors(m_qMapConnectors);

            QMap<qint32, QString>::const_iterator it;
            for(it = m_qMapConnectors.begin(); it != m_qMapConnectors.end(); ++it)
                if(it.value().compare("Neuromag Connector") == 0 && m_iActiveConnectorId != it.key())
                    changeConnector(it.key());

            //
            // Read Buffer Size
            //
            m_iBufferSize = m_pRtCmdClient->requestBufsize();

            emit cmdConnectionChanged(m_bCmdClientIsConnected);
        }
        m_mutex.unlock();
    }
}


//*************************************************************************************************************

void Neuromag::disconnectCmdClient()
{
    if(m_bCmdClientIsConnected)
    {
        m_pRtCmdClient->disconnectFromHost();
        m_pRtCmdClient->waitForDisconnected();
        m_mutex.lock();
        m_bCmdClientIsConnected = false;
        m_mutex.unlock();
        emit cmdConnectionChanged(m_bCmdClientIsConnected);
    }
}


//*************************************************************************************************************

void Neuromag::requestInfo()
{
    while(!(m_pNeuromagProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected))
        qWarning() << "NeuromagProducer is not running! Retry...";

    if(m_pNeuromagProducer->m_iDataClientId > -1 && m_bCmdClientIsConnected)
    {
        // read meas info
        (*m_pRtCmdClient)["measinfo"].pValues()[0].setValue(m_pNeuromagProducer->m_iDataClientId);
        (*m_pRtCmdClient)["measinfo"].send();

        m_pNeuromagProducer->m_mutex.lock();
        m_pNeuromagProducer->m_bFlagInfoRequest = true;
        m_pNeuromagProducer->m_mutex.unlock();
    }
    else
        qWarning() << "NeuromagProducer is not connected!";
}


//*************************************************************************************************************

void Neuromag::run()
{
    MatrixXf matValue;

    qint32 size = 0;

    while(m_bIsRunning) {
        if(m_pRawMatrixBuffer_In) {
            //pop matrix
            matValue = m_pRawMatrixBuffer_In->pop();

            //Write raw data to fif file
            if(m_bWriteToFile) {
                size += matValue.rows()*matValue.cols() * 4;

                if(size > MAX_DATA_LEN) {
                    size = 0;
                    this->splitRecordingFile();
                }

                m_mutex.lock();
                if(m_pOutfid) {
                    m_pOutfid->write_raw_buffer(matValue.cast<double>());
                }
                m_mutex.unlock();
            } else {
                size = 0;
            }

            if(m_pRTMSA_Neuromag) {
                m_pRTMSA_Neuromag->data()->setValue(this->calibrate(matValue));
            }
        }
    }
}


//*************************************************************************************************************

MatrixXd Neuromag::calibrate(const MatrixXf& data)
{
    MatrixXd one;
    if(m_pFiffInfo && m_sparseMatCals.cols() == m_pFiffInfo->nchan) {
        one = m_sparseMatCals*data.cast<double>();
    } else {
        one = data.cast<double>();
    }

    return one;
}


//*************************************************************************************************************

void Neuromag::changeRecordingButton()
{
    if(m_iBlinkStatus == 0) {
        m_pActionRecordFile->setIcon(QIcon(":/images/record.png"));
        m_iBlinkStatus = 1;
    } else {
        m_pActionRecordFile->setIcon(QIcon(":/images/record_active.png"));
        m_iBlinkStatus = 0;
    }
}


//*************************************************************************************************************

void Neuromag::onRecordingRemainingTimeChange()
{
    m_pProjectSettingsView->setRecordingElapsedTime(m_recordingStartedTime.elapsed());
}


//*************************************************************************************************************

void Neuromag::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSA_Neuromag = PluginOutputData<RealTimeMultiSampleArray>::create(this, "Realtime", "MNE Rt Client");

        m_pRTMSA_Neuromag->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSA_Neuromag->data()->setMultiArraySize(1);

        m_pRTMSA_Neuromag->data()->setVisibility(true);

        m_pRTMSA_Neuromag->data()->setXMLLayoutFile("./resources/mne_scan/plugins/Neuromag/VectorViewLayout.xml");

        m_outputConnectors.append(m_pRTMSA_Neuromag);

        //Add the calibration factors
        m_cals = RowVectorXd(m_pFiffInfo->nchan);
        m_cals.setZero();
        for (qint32 k = 0; k < m_pFiffInfo->nchan; ++k) {
            m_cals[k] = m_pFiffInfo->chs[k].range*m_pFiffInfo->chs[k].cal;
        }

        //Initialize the data and calibration vector
        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(m_pFiffInfo->nchan);
        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i) {
            tripletList.push_back(T(i, i, this->m_cals[i]));
        }

        m_sparseMatCals = SparseMatrix<double>(m_pFiffInfo->nchan, m_pFiffInfo->nchan);
        m_sparseMatCals.setFromTriplets(tripletList.begin(), tripletList.end());
    }
}


//*************************************************************************************************************

void Neuromag::showHPIDialog()
{
    if(!m_pFiffInfo) {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    } else {
        if (!m_pHPIWidget) {
            m_pHPIWidget = QSharedPointer<HpiView>(new HpiView(m_pFiffInfo));
        }

        if (!m_pHPIWidget->isVisible()) {
            m_pHPIWidget->show();
            m_pHPIWidget->raise();
        }
    }
}


//*************************************************************************************************************

void Neuromag::updateHPI(const MatrixXf& matData)
{
    if(m_pFiffInfo && m_pHPIWidget) {
        m_pHPIWidget->setData(this->calibrate(matData));
    }
}
