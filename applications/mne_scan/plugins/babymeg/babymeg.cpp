//=============================================================================================================
/**
 * @file     babymeg.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Gabriel B Motta, Limin Sun, Lorenz Esch. All rights reserved.
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
 * @brief    BabyMEG class definition.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"
#include "FormFiles/babymegsetupwidget.h"
#include "FormFiles/babymegsquidcontroldgl.h"
#include "babymegclient.h"
#include "babymeginfo.h"

#include <utils/ioutils.h>
#include <utils/detecttrigger.h>
#include <fiff/fiff_types.h>
#include <fiff/fiff_dig_point_set.h>
#include <disp/viewers/projectsettingsview.h>
#include <communication/rtClient/rtcmdclient.h>
#include <scMeas/realtimemultisamplearray.h>
#include <disp3D/viewers/hpiview.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QQuaternion>
#include <QFile>


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace IOBUFFER;
using namespace SCMEASLIB;
using namespace DISP3DLIB;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_iBlinkStatus(0)
, m_iBufferSize(-1)
, m_bWriteToFile(false)
, m_bIsRunning(false)
, m_bUseRecordTimer(false)
, m_pRawMatrixBuffer(0)
, m_sFiffProjections(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/babymeg/header.fif")
, m_sFiffCompensators(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/babymeg/compensator.fif")
, m_sBadChannels(QCoreApplication::applicationDirPath() + "/resources/mne_scan/plugins/babymeg/both.bad")
, m_iRecordingMSeconds(5*60*1000)
, m_iSplitCount(0)
, m_bDoContinousHPI(false)
{
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup Project"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionSetupProject->setStatusTip(tr("Setup Project"));
    connect(m_pActionSetupProject.data(), &QAction::triggered,
            this, &BabyMEG::showProjectDialog);
    addPluginAction(m_pActionSetupProject);    

    m_pActionRecordFile = new QAction(QIcon(":/images/record.png"), tr("Start Recording"),this);
    m_pActionRecordFile->setStatusTip(tr("Start Recording"));
    connect(m_pActionRecordFile.data(), &QAction::triggered,
            this, &BabyMEG::toggleRecordingFile);
    addPluginAction(m_pActionRecordFile);
    //m_pActionRecordFile->setEnabled(false);

    m_pActionSqdCtrl = new QAction(QIcon(":/images/sqdctrl.png"), tr("Squid Control"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionSqdCtrl->setStatusTip(tr("Squid Control"));
    connect(m_pActionSqdCtrl.data(), &QAction::triggered,
            this, &BabyMEG::showSqdCtrlDialog);
    addPluginAction(m_pActionSqdCtrl);

    m_pActionUpdateFiffInfo = new QAction(QIcon(":/images/latestFiffInfo.png"), tr("Update Fiff Info"),this);
    m_pActionUpdateFiffInfo->setStatusTip(tr("Update Fiff Info"));
    connect(m_pActionUpdateFiffInfo.data(), &QAction::triggered,
            this, &BabyMEG::updateFiffInfo);
    addPluginAction(m_pActionUpdateFiffInfo);

    //Init HPI
    m_pActionComputeHPI = new QAction(QIcon(":/images/latestFiffInfoHPI.png"), tr("Compute HPI"),this);
    m_pActionComputeHPI->setStatusTip(tr("Compute HPI"));
    connect(m_pActionComputeHPI.data(), &QAction::triggered,
            this, &BabyMEG::showHPIDialog);
    addPluginAction(m_pActionComputeHPI);

    //Init timers
    if(!m_pRecordTimer) {
        m_pRecordTimer = QSharedPointer<QTimer>(new QTimer(this));
        m_pRecordTimer->setSingleShot(true);
        connect(m_pRecordTimer.data(), &QTimer::timeout,
                this, &BabyMEG::toggleRecordingFile);
    }

    if(!m_pBlinkingRecordButtonTimer) {
        m_pBlinkingRecordButtonTimer = QSharedPointer<QTimer>(new QTimer(this));
        connect(m_pBlinkingRecordButtonTimer.data(), &QTimer::timeout,
                this, &BabyMEG::changeRecordingButton);
    }

    if(!m_pUpdateTimeInfoTimer) {
        m_pUpdateTimeInfoTimer = QSharedPointer<QTimer>(new QTimer(this));
        connect(m_pUpdateTimeInfoTimer.data(), &QTimer::timeout,
                this, &BabyMEG::onRecordingRemainingTimeChange);
    }
}


//*************************************************************************************************************

BabyMEG::~BabyMEG()
{
    if(this->isRunning())
        stop();

    if(m_pMyClient) {
        if(m_pMyClient->isConnected()) {
            m_pMyClient->DisConnectBabyMEG();
        }
    }
}


//*************************************************************************************************************

QSharedPointer<IPlugin> BabyMEG::clone() const
{
    QSharedPointer<BabyMEG> pBabyMEGClone(new BabyMEG());
    return pBabyMEGClone;
}


//*************************************************************************************************************

void BabyMEG::init()
{
    //BabyMEGData Path
    QString sBabyMEGDataPath = QDir::homePath() + "/BabyMEGData";
    if(!QDir(sBabyMEGDataPath).exists()) {
        QDir().mkdir(sBabyMEGDataPath);
    }

    //Test Project
    QSettings settings;
    QString sCurrentProject = settings.value(QString("Plugin/%1/currentProject").arg(getName()), "TestProject").toString();
    if(!QDir(sBabyMEGDataPath+"/"+sCurrentProject).exists()) {
        QDir().mkdir(sBabyMEGDataPath+"/"+sCurrentProject);
    }

    //Test Subject
    QString sCurrentSubject = settings.value(QString("Plugin/%1/currentSubject").arg(getName()), "TestSubject").toString();
    if(!QDir(sBabyMEGDataPath+"/"+sCurrentProject+"/"+sCurrentSubject).exists()) {
        QDir().mkdir(sBabyMEGDataPath+"/"+sCurrentProject+"/"+sCurrentSubject);
    }

    m_pProjectSettingsView = QSharedPointer<ProjectSettingsView>::create(sBabyMEGDataPath,
                                                                         sCurrentProject,
                                                                         sCurrentSubject,
                                                                         "");
    m_sRecordFile = m_pProjectSettingsView->getCurrentFileName();

    connect(m_pProjectSettingsView.data(), &ProjectSettingsView::timerChanged,
            this, &BabyMEG::setRecordingTimerChanged);

    connect(m_pProjectSettingsView.data(), &ProjectSettingsView::recordingTimerStateChanged,
            this, &BabyMEG::setRecordingTimerStateChanged);

    m_pProjectSettingsView->hide();

    //BabyMEG Inits
    m_pInfo = QSharedPointer<BabyMEGInfo>(new BabyMEGInfo());
    connect(m_pInfo.data(), &BabyMEGInfo::fiffInfoAvailable,
            this, &BabyMEG::setFiffInfo);
    connect(m_pInfo.data(), &BabyMEGInfo::SendDataPackage,
            this, &BabyMEG::setFiffData);
    connect(m_pInfo.data(), &BabyMEGInfo::SendCMDPackage,
            this, &BabyMEG::setCMDData);
    connect(m_pInfo.data(), &BabyMEGInfo::GainInfoUpdate,
            this, &BabyMEG::setFiffGainInfo);

    m_pMyClient = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6340,this));
    m_pMyClient->SetInfo(m_pInfo);
    m_pMyClient->start();
    m_pMyClientComm = QSharedPointer<BabyMEGClient>(new BabyMEGClient(6341,this));
    m_pMyClientComm->SetInfo(m_pInfo);
    m_pMyClientComm->start();

    m_pMyClientComm->SendCommandToBabyMEGShortConnection("INFO");

    m_pMyClient->ConnectToBabyMEG();

    //init channels when fiff info is available
    connect(this, &BabyMEG::fiffInfoAvailable,
            this, &BabyMEG::initConnector);
}


//*************************************************************************************************************

void BabyMEG::unload()
{

}


//*************************************************************************************************************

void BabyMEG::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}


//*************************************************************************************************************

bool BabyMEG::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    if(!m_pRTMSABabyMEG)
        initConnector();

    // Start threads
    m_bIsRunning = true;

    if(!m_pMyClient->isConnected())
        m_pMyClient->ConnectToBabyMEG();
    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::stop()
{
    if(m_pMyClient->isConnected())
        m_pMyClient->DisConnectBabyMEG();

    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer->releaseFromPop();

    //Clear Buffers
    m_pRawMatrixBuffer->clear();

    if(m_pHPIWidget) {
        m_pHPIWidget->hide();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType BabyMEG::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString BabyMEG::getName() const
{
    return "BabyMEG";
}


//*************************************************************************************************************

QWidget* BabyMEG::setupWidget()
{
    if(!m_pMyClient->isConnected())
        m_pMyClient->ConnectToBabyMEG();

    BabyMEGSetupWidget* widget = new BabyMEGSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    return widget;
}


//*************************************************************************************************************

void BabyMEG::run()
{
    MatrixXf matValue;
    qint32 size = 0;

    while(m_bIsRunning) {
        if(m_pRawMatrixBuffer) {
            //pop matrix
            matValue = m_pRawMatrixBuffer->pop();

            //Update HPI data (for single and continous HPI fitting)
            updateHPI(matValue);

            //Do continous HPI fitting and write result to data block
            if(m_bDoContinousHPI) {
                doContinousHPI(matValue);
            }

            //Create digital trigger information
            createDigTrig(matValue);

            //Write raw data to fif file
            if(m_bWriteToFile) {
                size += matValue.rows()*matValue.cols() * 4;

                if(size > MAX_DATA_LEN) {
                    size = 0;
                    this->splitRecordingFile();
                }

                m_mutex.lock();
                m_pOutfid->write_raw_buffer(matValue.cast<double>());
                m_mutex.unlock();
            } else {
                size = 0;
            }

            if(m_pRTMSABabyMEG) {
                m_pRTMSABabyMEG->data()->setValue(this->calibrate(matValue));
            }
        }
    }

    //Close the fif output stream
    if(m_bWriteToFile) {
        this->toggleRecordingFile();
    }
}


//*************************************************************************************************************

void BabyMEG::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pRTMSABabyMEG = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BabyMEG Output", "BabyMEG");
        m_pRTMSABabyMEG->data()->setName(this->getName());//Provide name to auto store widget settings

        m_pRTMSABabyMEG->data()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSABabyMEG->data()->setMultiArraySize(1);

        m_pRTMSABabyMEG->data()->setSamplingRate(m_pFiffInfo->sfreq);

        m_pRTMSABabyMEG->data()->setVisibility(true);

        m_outputConnectors.append(m_pRTMSABabyMEG);

        //Look for trigger channels and initialise detected trigger map
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG001"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG002"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG003"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG004"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG005"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG006"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG007"));
        m_lTriggerChannelIndices.append(m_pFiffInfo->ch_names.indexOf("TRG008"));
    }
}


//*************************************************************************************************************

void BabyMEG::setFiffInfo(const FiffInfo& p_FiffInfo)
{
    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(p_FiffInfo));

    if(!readProjectors())
    {
        qDebug() << "Not able to read projectors";
    }

    if(!readCompensators())
    {
        qDebug() << "Not able to read compensators";
    }

    if(!readBadChannels())
    {
        qDebug() << "Not able to read bad channels";
    }

    m_iBufferSize = m_pInfo->dataLength;

    //
    //   Add the calibration factors
    //
    m_cals = RowVectorXd(m_pFiffInfo->nchan);
    m_cals.setZero();
    for (qint32 k = 0; k < m_pFiffInfo->nchan; ++k)
        m_cals[k] = m_pFiffInfo->chs[k].range*m_pFiffInfo->chs[k].cal;

    //
    //  Initialize the data and calibration vector
    //
    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(m_pFiffInfo->nchan);
    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        tripletList.push_back(T(i, i, this->m_cals[i]));

    m_sparseMatCals = SparseMatrix<double>(m_pFiffInfo->nchan, m_pFiffInfo->nchan);
    m_sparseMatCals.setFromTriplets(tripletList.begin(), tripletList.end());

    emit fiffInfoAvailable();
}


//*************************************************************************************************************

void BabyMEG::setFiffData(QByteArray data)
{
    //get the first byte -- the data format
    int dformat = data.left(1).toInt();

    data.remove(0,1);
    qint32 rows = m_pFiffInfo->nchan;
    qint32 cols = (data.size()/dformat)/rows;

    qDebug() << "[BabyMEG] Matrix " << rows << "x" << cols << " [Data bytes:" << dformat << "]";

    MatrixXf rawData(Map<MatrixXf>( (float*)data.data(),rows, cols ));

    for(qint32 i = 0; i < rows*cols; ++i)
        IOUtils::swap_floatp(rawData.data()+i);


    if(m_bIsRunning)
    {
        if(!m_pRawMatrixBuffer)
            m_pRawMatrixBuffer = CircularMatrixBuffer<float>::SPtr(new CircularMatrixBuffer<float>(40, rows, cols));

        m_pRawMatrixBuffer->push(&rawData);
    }

    emit dataToSquidCtrlGUI(rawData);
}


//*************************************************************************************************************

void BabyMEG::setCMDData(QByteArray DATA)
{
    qDebug()<<"------"<<DATA;
//    m_commandManager["FLL"].reply(DATA);
    emit sendCMDDataToSQUIDControl(DATA);
    qDebug()<<"Data has been received.";
}


//*************************************************************************************************************

void BabyMEG::setFiffGainInfo(QStringList GainInfo)
{
    if(!m_pFiffInfo) {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    } else {
        //set up the gain info
        qDebug()<<"Set Gain Info";
        for(qint32 i = 0; i < m_pFiffInfo->nchan; i++) {
            m_pFiffInfo->chs[i].range = 1.0f/GainInfo.at(i).toFloat();//1; // set gain
            m_cals[i] = m_pFiffInfo->chs[i].range*m_pFiffInfo->chs[i].cal;
            //qDebug()<<i<<"="<<m_pFiffInfo->chs[i].ch_name<<","<<m_pFiffInfo->chs[i].range;
        }

        // Initialize the data and calibration vector
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

void BabyMEG::comFLL(QString t_sFLLControlCommand)
{
    qDebug()<<"FLL commands";

    qDebug() << "BabyMeg Received" << t_sFLLControlCommand;
    int strlen = t_sFLLControlCommand.size();
    QByteArray Scmd = m_pMyClientComm->MGH_LM_Int2Byte(strlen);
    QByteArray SC = QByteArray("COMS")+Scmd;
    SC.append(t_sFLLControlCommand);
    m_pMyClientComm->SendCommandToBabyMEGShortConnection(SC);
}


//*************************************************************************************************************

void BabyMEG::updateFiffInfo()
{
    // read gain info and save them to the m_pFiffInfo.range
    m_pMyClientComm->SendCommandToBabyMEGShortConnection("INFG");

    //sleep(0.5);

    //m_pActionRecordFile->setEnabled(true);

}


//*************************************************************************************************************

void BabyMEG::showHPIDialog()
{
    if(!m_pFiffInfo) {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    } else {
        if (!m_pHPIWidget) {
            m_pHPIWidget = QSharedPointer<HpiView>(new HpiView(m_pFiffInfo));
            connect(m_pHPIWidget.data(), &HpiView::continousHPIToggled,
                    this, &BabyMEG::onContinousHPIToggled);
        }

        if (!m_pHPIWidget->isVisible()) {
            m_pHPIWidget->show();
            m_pHPIWidget->raise();
        }
    }
}


//*************************************************************************************************************

void BabyMEG::updateHPI(const MatrixXf& matData)
{
    if(m_pFiffInfo && m_pHPIWidget) {
        m_pHPIWidget->setData(this->calibrate(matData));
    }
}


//*************************************************************************************************************

void BabyMEG::doContinousHPI(MatrixXf& matData)
{
    //This only works with babyMEG HPI channels 400 ... 407
    if(m_pFiffInfo && m_pHPIWidget && matData.rows() >= 407) {
        //if(m_pHPIWidget->wasLastFitOk()) {
            // Load device to head transformation matrix from Fiff info
            QMatrix3x3 rot;

            for(int ir = 0; ir < 3; ir++) {
                for(int ic = 0; ic < 3; ic++) {
                    rot(ir,ic) = m_pFiffInfo->dev_head_t.trans(ir,ic);
                }
            }

            QQuaternion quatHPI = QQuaternion::fromRotationMatrix(rot);

            // Write goodness of fit (GOF)to HPI Ch #7
//            float dpfitError = 0.0;
//            float GOF = 1 - dpfitError;
            QVector<double> vGof = m_pHPIWidget->getGOF();
            float GOF = 0.0f;
            for(int i = 0; i < vGof.size(); ++i) {
                GOF += vGof.at(i);
            }
            GOF = GOF / vGof.size();

            // Write rotation quaternion to HPI Ch #1~3
            matData.row(401) = MatrixXf::Constant(1,matData.cols(), quatHPI.x());
            matData.row(402) = MatrixXf::Constant(1,matData.cols(), quatHPI.y());
            matData.row(403) = MatrixXf::Constant(1,matData.cols(), quatHPI.z());

            // Write translation vector to HPI Ch #4~6
            matData.row(404) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(0,3));
            matData.row(405) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(1,3));
            matData.row(406) = MatrixXf::Constant(1,matData.cols(), m_pFiffInfo->dev_head_t.trans(2,3));

            // Write GOF to HPI Ch #7
            matData.row(407) = MatrixXf::Constant(1,matData.cols(), GOF);
        //}
    }
}


//*************************************************************************************************************

void BabyMEG::onContinousHPIToggled(bool bDoContinousHPI)
{
    m_bDoContinousHPI = bDoContinousHPI;
}


//*************************************************************************************************************

void BabyMEG::setRecordingTimerChanged(int timeMSecs)
{
    //If the recording time is changed during the recording, change the timer
    if(m_bWriteToFile)
        m_pRecordTimer->setInterval(timeMSecs-m_recordingStartedTime.elapsed());

    m_iRecordingMSeconds = timeMSecs;
}


//*************************************************************************************************************

void BabyMEG::setRecordingTimerStateChanged(bool state)
{
    m_bUseRecordTimer = state;
}


//*************************************************************************************************************

void BabyMEG::setFileName(const QString& sFileName)
{
    m_sRecordFile = sFileName;
}


//*************************************************************************************************************

void BabyMEG::showProjectDialog()
{
    if(m_pProjectSettingsView) {
        m_pProjectSettingsView->show();
    }
}


//*************************************************************************************************************

void BabyMEG::showSqdCtrlDialog()
{
    // Start Squid control widget
    if(!m_pSQUIDCtrlDlg) {
        m_pSQUIDCtrlDlg = QSharedPointer<BabyMEGSQUIDControlDgl>(new BabyMEGSQUIDControlDgl(this));
    }

    if(!m_pSQUIDCtrlDlg->isVisible()) {
        m_pSQUIDCtrlDlg->show();
        m_pSQUIDCtrlDlg->raise();
        m_pSQUIDCtrlDlg->Init();
    }
}


//*************************************************************************************************************

void BabyMEG::splitRecordingFile()
{
    //qDebug() << "Split recording file";
    ++m_iSplitCount;
    QString nextFileName = m_sRecordFile.remove("_raw.fif");
    nextFileName += QString("-%1_raw.fif").arg(m_iSplitCount);

    //Write the link to the next file
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
    MatrixXi sel;
    m_pOutfid = FiffStream::start_writing_raw(m_qFileOut,
                                              *m_pFiffInfo,
                                              cals,
                                              sel,
                                              false);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
}


//*************************************************************************************************************

void BabyMEG::toggleRecordingFile()
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
            if(ret == QMessageBox::No) {
                return;
            }
        }

        //Set all projectors to zero before writing to file because we always write the raw data
        for(int i = 0; i<m_pFiffInfo->projs.size(); i++) {
            m_pFiffInfo->projs[i].active = false;
        }

        //Start/Prepare writing process. Actual writing is done in run() method.
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

void BabyMEG::createDigTrig(MatrixXf& data)
{
    //Look for triggers in all trigger channels
    //m_qMapDetectedTrigger = DetectTrigger::detectTriggerFlanksMax(data.at(b), m_lTriggerChannelIndices, m_iCurrentSample-nCol, m_dTriggerThreshold, true);
    QMap<int,QList<QPair<int,double> > > qMapDetectedTrigger = DetectTrigger::detectTriggerFlanksGrad(data.cast<double>(),
                                                                                                      m_lTriggerChannelIndices,
                                                                                                      0,
                                                                                                      1.0,
                                                                                                      false,
                                                                                                      "Rising",
                                                                                                      400);

    //Combine and write results into data block's digital trigger channel
    QMapIterator<int,QList<QPair<int,double> > > i(qMapDetectedTrigger);
    int counter = 0;
    int idxDigTrig = m_pFiffInfo->ch_names.indexOf("DTRG01");

    while (i.hasNext()) {
        i.next();

        QList<QPair<int,double> > lDetectedTriggers = i.value();

        for(int k = 0; k < lDetectedTriggers.size(); ++k) {
            if(lDetectedTriggers.at(k).first < data.cols() && lDetectedTriggers.at(k).first >= 0) {
                data(idxDigTrig,lDetectedTriggers.at(k).first) = data(idxDigTrig,lDetectedTriggers.at(k).first) + pow(2,counter);
            }
        }

        counter++;
    }
}


//*************************************************************************************************************

MatrixXd BabyMEG::calibrate(const MatrixXf& data)
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

bool BabyMEG::readProjectors()
{
    QFile t_projFiffFile(m_sFiffProjections);

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&t_projFiffFile));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening header data %s...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()) {
        return false;
    }

    QList<FiffProj> q_ListProj = t_pStream->read_proj(t_pStream->dirtree());

    //Set all projectors to zero
    for(int i = 0; i<q_ListProj.size(); i++)
        q_ListProj[i].active = false;

    if (q_ListProj.size() == 0) {
        printf("Could not find projectors\n");
        return false;
    }

    m_pFiffInfo->projs = q_ListProj;

    //garbage collecting
    t_pStream->close();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::readCompensators()
{
    QFile t_compFiffFile(m_sFiffCompensators);

    //
    //   Open the file
    //
    FiffStream::SPtr t_pStream(new FiffStream(&t_compFiffFile));
    QString t_sFileName = t_pStream->streamName();

    printf("Opening compensator data %s...\n",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()) {
        return false;
    }

    QList<FiffCtfComp> q_ListComp = t_pStream->read_ctf_comp(t_pStream->dirtree(), m_pFiffInfo->chs);

    if (q_ListComp.size() == 0) {
        printf("Could not find compensators\n");
        return false;
    }

    m_pFiffInfo->comps = q_ListComp;

    //garbage collecting
    t_pStream->close();

    return true;
}


//*************************************************************************************************************

bool BabyMEG::readBadChannels()
{
    //
    // Bad Channels
    //
//    //Read bad channels from header/projection fif
//    QFile t_headerFiffFile(m_sFiffProjections);

//    if(!t_headerFiffFile.exists()) {
//        printf("Could not open fif file for copying bad channels to babyMEG fiff_info\n");
//        return false;
//    }

//    FiffRawData raw(t_headerFiffFile);
//    m_pFiffInfo->bads = raw.info.bads;

//    t_headerFiffFile.close();

//    return true;

    //Read bad channels from
    QFile t_badChannelsFile(m_sBadChannels);

    if (!t_badChannelsFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    printf("Reading bad channels from %s...\n", m_sBadChannels.toUtf8().constData());

    QTextStream in(&t_badChannelsFile);
    qint32 count = 0;
    QStringList t_sListbads;
    while (!in.atEnd()) {
        QString channel = in.readLine();
        if(channel.isEmpty())
            continue;
        ++count;
        printf("Channel %i: %s\n",count,channel.toUtf8().constData());
        t_sListbads << channel;
    }

    m_pFiffInfo->bads = t_sListbads;

    return true;
}


//*************************************************************************************************************

void BabyMEG::changeRecordingButton()
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

void BabyMEG::onRecordingRemainingTimeChange()
{
    m_pProjectSettingsView->setRecordingElapsedTime(m_recordingStartedTime.elapsed());
}


