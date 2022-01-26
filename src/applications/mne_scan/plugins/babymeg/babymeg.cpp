//=============================================================================================================
/**
 * @file     babymeg.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "babymeg.h"
#include "FormFiles/babymegsetupwidget.h"
#include "FormFiles/babymegsquidcontroldgl.h"
#include "babymegclient.h"
#include "babymeginfo.h"

#include <utils/ioutils.h>
#include <rtprocessing/detecttrigger.h>
#include <fiff/fiff_types.h>
#include <fiff/fiff_dig_point_set.h>
#include <disp/viewers/projectsettingsview.h>
#include <communication/rtClient/rtcmdclient.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QQuaternion>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BABYMEGPLUGIN;
using namespace UTILSLIB;
using namespace SCSHAREDLIB;
using namespace UTILSLIB;
using namespace SCMEASLIB;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace Eigen;
using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BabyMEG::BabyMEG()
: m_pCircularBuffer(CircularBuffer_Matrix_float::SPtr(new CircularBuffer_Matrix_float(40)))
, m_iBufferSize(-1)
, m_sFiffProjections(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/babymeg/header.fif")
, m_sFiffCompensators(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/babymeg/compensator.fif")
, m_sBadChannels(QCoreApplication::applicationDirPath() + "/../resources/mne_scan/plugins/babymeg/both.bad")
, m_bProcessOutput(false)
{
    m_pActionSqdCtrl = new QAction(QIcon(":/images/sqdctrl.png"), tr("Squid Control"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionSqdCtrl->setStatusTip(tr("Squid Control"));
    connect(m_pActionSqdCtrl.data(), &QAction::triggered,
            this, &BabyMEG::showSqdCtrlDialog);
    //addPluginAction(m_pActionSqdCtrl);

    m_pActionUpdateFiffInfo = new QAction(QIcon(":/images/latestFiffInfo.png"), tr("Update Fiff Info"),this);
    m_pActionUpdateFiffInfo->setStatusTip(tr("Update Fiff Info"));
    connect(m_pActionUpdateFiffInfo.data(), &QAction::triggered,
            this, &BabyMEG::updateFiffInfo);
    addPluginAction(m_pActionUpdateFiffInfo);
}

//=============================================================================================================

BabyMEG::~BabyMEG()
{
    if(m_bProcessOutput) {
        stop();
    }

    if(m_pMyClient) {
        if(m_pMyClient->isConnected()) {
            m_pMyClient->DisconnectBabyMEG();
        }
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> BabyMEG::clone() const
{
    QSharedPointer<BabyMEG> pBabyMEGClone(new BabyMEG());
    return std::move(pBabyMEGClone);
}

//=============================================================================================================

void BabyMEG::init()
{
    //BabyMEGData Path
    QString sBabyMEGDataPath = QDir::homePath() + "/BabyMEGData";
    if(!QDir(sBabyMEGDataPath).exists()) {
        QDir().mkdir(sBabyMEGDataPath);
    }

    //Test Project
    QSettings settings("MNECPP");
    QString sCurrentProject = settings.value(QString("Plugin/%1/currentProject").arg(getName()), "TestProject").toString();
    if(!QDir(sBabyMEGDataPath+"/"+sCurrentProject).exists()) {
        QDir().mkdir(sBabyMEGDataPath+"/"+sCurrentProject);
    }

    //Test Subject
    QString sCurrentSubject = settings.value(QString("Plugin/%1/currentSubject").arg(getName()), "TestSubject").toString();
    if(!QDir(sBabyMEGDataPath+"/"+sCurrentProject+"/"+sCurrentSubject).exists()) {
        QDir().mkdir(sBabyMEGDataPath+"/"+sCurrentProject+"/"+sCurrentSubject);
    }

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

    //init channels when fiff info is available
    connect(this, &BabyMEG::fiffInfoAvailable,
            this, &BabyMEG::initConnector);    

    m_pRTMSABabyMEG = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BabyMEG Output", "BabyMEG");
    m_pRTMSABabyMEG->measurementData()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSABabyMEG);
}

//=============================================================================================================

void BabyMEG::unload()
{
}

//=============================================================================================================

void BabyMEG::clear()
{
    m_pFiffInfo.reset();
    m_iBufferSize = -1;
}

//=============================================================================================================

bool BabyMEG::start()
{
    m_pMyClientComm->SendCommandToBabyMEGShortConnection("INFO");

    m_pMyClient->ConnectToBabyMEG();

    if(!m_pMyClient->isConnected()) {
        qInfo() << "BabyMEG::start - Not connected to BabyMEG device. Try connecting manually via the Connection tab.";

        return false;
    }

    initConnector();

    // Start thread
    m_bProcessOutput = true;
    m_OutputProcessingThread = std::thread(&BabyMEG::run, this);

    return true;
}

//=============================================================================================================

bool BabyMEG::stop()
{
    m_bProcessOutput = false;

    if(m_OutputProcessingThread.joinable()){
        m_OutputProcessingThread.join();
    }

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSABabyMEG->measurementData()->clear();
    m_pCircularBuffer->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType BabyMEG::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString BabyMEG::getName() const
{
    return "BabyMEG";
}

//=============================================================================================================

QWidget* BabyMEG::setupWidget()
{
    BabyMEGSetupWidget* widget = new BabyMEGSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    if(m_pFiffInfo) {
        widget->setSamplingFrequency();
    }

    return widget;
}

//=============================================================================================================

void BabyMEG::run()
{
    MatrixXf matValue;

    while(m_bProcessOutput) {
        //pop matrix
        if(m_pCircularBuffer->pop(matValue)) {
            //Create digital trigger information
            createDigTrig(matValue);

            if(m_bProcessOutput) {
                m_pRTMSABabyMEG->measurementData()->setValue(this->calibrate(matValue));
            }
        }
    }
}

//=============================================================================================================

void BabyMEG::initConnector()
{
    if(m_pFiffInfo) {
        m_pRTMSABabyMEG->measurementData()->initFromFiffInfo(m_pFiffInfo);
        m_pRTMSABabyMEG->measurementData()->setMultiArraySize(1);

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

//=============================================================================================================

void BabyMEG::setFiffInfo(const FiffInfo& p_FiffInfo)
{
    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo(p_FiffInfo));

    if(!readProjectors()) {
        qWarning() << "[BabyMEG::setFiffInfo] Not able to read projectors";
    }

    if(!readCompensators()) {
        qWarning() << "[BabyMEG::setFiffInfo] Not able to read compensators";
    }

    if(!readBadChannels()) {
        qWarning() << "[BabyMEG::setFiffInfo] Not able to read bad channels";
    }

    m_iBufferSize = m_pInfo->dataLength;

    // Add the calibration factors
    m_cals = RowVectorXd(m_pFiffInfo->nchan);
    m_cals.setZero();
    for (qint32 k = 0; k < m_pFiffInfo->nchan; ++k)
        m_cals[k] = static_cast<double>(m_pFiffInfo->chs[k].range) *
                    static_cast<double>(m_pFiffInfo->chs[k].cal);

    // Initialize the data and calibration vector
    typedef Eigen::Triplet<double> T;
    std::vector<T> tripletList;
    tripletList.reserve(static_cast<unsigned long>(m_pFiffInfo->nchan));
    for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        tripletList.push_back(T(i, i, this->m_cals[i]));

    m_sparseMatCals = SparseMatrix<double>(m_pFiffInfo->nchan, m_pFiffInfo->nchan);
    m_sparseMatCals.setFromTriplets(tripletList.begin(), tripletList.end());

    emit fiffInfoAvailable();
}

//=============================================================================================================

void BabyMEG::setFiffData(QByteArray data)
{
    //get the first byte -- the data format
    int dformat = data.left(1).toInt();

    data.remove(0,1);
    qint32 rows = m_pFiffInfo->nchan;
    qint32 cols = (data.size()/dformat)/rows;

    qInfo() << "[BabyMEG::setFiffData] Matrix " << rows << "x" << cols << " [Data bytes:" << dformat << "]";

    MatrixXf rawData(Map<MatrixXf>( reinterpret_cast<float*>(data.data()),rows, cols ));

    for(qint32 i = 0; i < rows*cols; ++i) {
        IOUtils::swap_floatp(rawData.data()+i);
    }

    if(m_bProcessOutput) {
        while(!m_pCircularBuffer->push(rawData)) {
            //Do nothing until the circular buffer is ready to accept new data again
        }
    }

    emit dataToSquidCtrlGUI(rawData);
}

//=============================================================================================================

void BabyMEG::setCMDData(QByteArray DATA)
{
//    m_commandManager["FLL"].reply(DATA);
    emit sendCMDDataToSQUIDControl(DATA);
    qInfo()<<"[BabyMEG::setCMDData] Data has been received.";
}

//=============================================================================================================

void BabyMEG::setFiffGainInfo(QStringList GainInfo)
{
    if(!m_pFiffInfo) {
        QMessageBox msgBox;
        msgBox.setText("FiffInfo missing!");
        msgBox.exec();
        return;
    } else {
        //set up the gain info
        qInfo()<<"Set Gain Info";
        for(qint32 i = 0; i < m_pFiffInfo->nchan; i++) {
            m_pFiffInfo->chs[i].range = 1.0f/GainInfo.at(i).toFloat();//1; // set gain
            m_cals[i] = static_cast<double>(m_pFiffInfo->chs[i].range * m_pFiffInfo->chs[i].cal);
            //qInfo()<<[BabyMEG::setFiffGainInfo] i<<"="<<m_pFiffInfo->chs[i].ch_name<<","<<m_pFiffInfo->chs[i].range;
        }

        // Initialize the data and calibration vector
        typedef Eigen::Triplet<double> T;
        std::vector<T> tripletList;
        tripletList.reserve(static_cast<unsigned long>(m_pFiffInfo->nchan));
        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i) {
            tripletList.push_back(T(i, i, this->m_cals[i]));
        }

        m_sparseMatCals = SparseMatrix<double>(m_pFiffInfo->nchan, m_pFiffInfo->nchan);
        m_sparseMatCals.setFromTriplets(tripletList.begin(), tripletList.end());
    }
}

//=============================================================================================================

void BabyMEG::comFLL(QString t_sFLLControlCommand)
{
    qInfo() << "[BabyMEG::comFLL] FLL commands";

    qInfo() << "[BabyMEG::comFLL] BabyMeg Received" << t_sFLLControlCommand;
    int strlen = t_sFLLControlCommand.size();
    QByteArray Scmd = m_pMyClientComm->MGH_LM_Int2Byte(strlen);
    QByteArray SC = QByteArray("COMS")+Scmd;
    SC.append(t_sFLLControlCommand);
    m_pMyClientComm->SendCommandToBabyMEGShortConnection(SC);
}

//=============================================================================================================

void BabyMEG::updateFiffInfo()
{
    // read gain info and save them to the m_pFiffInfo.range
    m_pMyClientComm->SendCommandToBabyMEGShortConnection("INFG");

    //sleep(0.5);

    //m_pActionRecordFile->setEnabled(true);
}

//=============================================================================================================

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

//=============================================================================================================

void BabyMEG::createDigTrig(MatrixXf& data)
{
    //Look for triggers in all trigger channels
    //m_qMapDetectedTrigger = RTPROCESSINGLIB::detectTriggerFlanksMax(data.at(b), m_lTriggerChannelIndices, m_iCurrentSample-nCol, m_dTriggerThreshold, true);
    QMap<int,QList<QPair<int,double> > > qMapDetectedTrigger = RTPROCESSINGLIB::detectTriggerFlanksGrad(data.cast<double>(),
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
                data(idxDigTrig,lDetectedTriggers.at(k).first) = data(idxDigTrig,lDetectedTriggers.at(k).first) + powf(2,counter);
            }
        }

        counter++;
    }
}

//=============================================================================================================

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

//=============================================================================================================

bool BabyMEG::readProjectors()
{
    QFile t_projFiffFile(m_sFiffProjections);

    // Open the file
    FiffStream::SPtr t_pStream(new FiffStream(&t_projFiffFile));
    QString t_sFileName = t_pStream->streamName();

    qInfo("[BabyMEG::readProjectors] Opening header data %s...",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()) {
        return false;
    }

    QList<FiffProj> q_ListProj = t_pStream->read_proj(t_pStream->dirtree());

    //Set all projectors to zero
    for(int i = 0; i<q_ListProj.size(); i++)
        q_ListProj[i].active = false;

    if (q_ListProj.size() == 0) {
        qInfo("[BabyMEG::readProjectors] Could not find projectors");
        return false;
    }

    m_pFiffInfo->projs = q_ListProj;

    //garbage collecting
    t_pStream->close();

    return true;
}

//=============================================================================================================

bool BabyMEG::readCompensators()
{
    QFile t_compFiffFile(m_sFiffCompensators);

    // Open the file
    FiffStream::SPtr t_pStream(new FiffStream(&t_compFiffFile));
    QString t_sFileName = t_pStream->streamName();

    qInfo("[BabyMEG::readCompensators] Opening compensator data %s...",t_sFileName.toUtf8().constData());

    if(!t_pStream->open()) {
        return false;
    }

    QList<FiffCtfComp> q_ListComp = t_pStream->read_ctf_comp(t_pStream->dirtree(), m_pFiffInfo->chs);

    if (q_ListComp.size() == 0) {
        qInfo("[BabyMEG::readCompensators] Could not find compensators");
        return false;
    }

    m_pFiffInfo->comps = q_ListComp;

    //garbage collecting
    t_pStream->close();

    return true;
}

//=============================================================================================================

bool BabyMEG::readBadChannels()
{
    // Bad Channels
//    //Read bad channels from header/projection fif
//    QFile t_headerFiffFile(m_sFiffProjections);

//    if(!t_headerFiffFile.exists()) {
//        qInfo("[BabyMEG::readBadChannels] Could not open fif file for copying bad channels to babyMEG fiff_info");
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

    qInfo("[BabyMEG::readBadChannels] Reading bad channels from %s...", m_sBadChannels.toUtf8().constData());

    QTextStream in(&t_badChannelsFile);
    qint32 count = 0;
    QStringList t_sListbads;
    while (!in.atEnd()) {
        QString channel = in.readLine();
        if(channel.isEmpty())
            continue;
        ++count;
        qInfo("[BabyMEG::readBadChannels] Channel %i: %s",count,channel.toUtf8().constData());
        t_sListbads << channel;
    }

    m_pFiffInfo->bads = t_sListbads;

    return true;
}

//=============================================================================================================

QString BabyMEG::getBuildInfo()
{
    return QString(BABYMEGPLUGIN::buildDateTime()) + QString(" - ")  + QString(BABYMEGPLUGIN::buildHash());
}
