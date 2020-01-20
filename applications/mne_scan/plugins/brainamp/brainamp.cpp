//=============================================================================================================
/**
 * @file     brainamp.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     October, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the BrainAMP class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainamp.h"
#include "brainampproducer.h"

#include "FormFiles/brainampsetupwidget.h"
#include "FormFiles/brainampsetupprojectwidget.h"

#include <direct.h>

#include <fiff/fiff.h>
#include <scMeas/realtimemultisamplearray.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BRAINAMPPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace FIFFLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAMP::BrainAMP()
: m_pRMTSA_BrainAMP(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/brainamp/")
, m_pRawMatrixBuffer_In(0)
, m_pBrainAMPProducer(new BrainAMPProducer(this))
, m_dLPAShift(0.01)
, m_dRPAShift(0.01)
, m_dNasionShift(0.06)
, m_bUseTrackedCardinalMode(true)
, m_bUseElectrodeShiftMode(false)
, m_sLPA("2LD")
, m_sRPA("2RD")
, m_sNasion("0Z")
{
    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &BrainAMP::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create start recordin action bar item/button
    m_pActionStartRecording = new QAction(QIcon(":/images/record.png"), tr("Start recording data to fif file"), this);
    m_pActionStartRecording->setStatusTip(tr("Start recording data to fif file"));
    connect(m_pActionStartRecording, &QAction::triggered, this, &BrainAMP::showStartRecording);
    addPluginAction(m_pActionStartRecording);
}


//*************************************************************************************************************

BrainAMP::~BrainAMP()
{
    //std::cout << "BrainAMP::~BrainAMP() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();    

    //Store settings for next use
    QSettings settings;
    settings.setValue(QString("BRAINAMP/sFreq"), m_iSamplingFreq);
    settings.setValue(QString("BRAINAMP/samplesPerBlock"), m_iSamplesPerBlock);
    settings.setValue(QString("BRAINAMP/LPAShift"), m_dLPAShift);
    settings.setValue(QString("BRAINAMP/RPAShift"), m_dRPAShift);
    settings.setValue(QString("BRAINAMP/NasionShift"), m_dNasionShift);
    settings.setValue(QString("BRAINAMP/LPAElectrode"), m_sLPA);
    settings.setValue(QString("BRAINAMP/RPAElectrode"), m_sRPA);
    settings.setValue(QString("BRAINAMP/NasionElectrode"), m_sNasion);
    settings.setValue(QString("BRAINAMP/outputFilePath"), m_sOutputFilePath);
    settings.setValue(QString("BRAINAMP/elcFilePath"), m_sElcFilePath);
    settings.setValue(QString("BRAINAMP/cardinalFilePath"), m_sCardinalFilePath);
    settings.setValue(QString("BRAINAMP/useTrackedCardinalsMode"), m_bUseTrackedCardinalMode);
    settings.setValue(QString("BRAINAMP/useElectrodeshiftMode"), m_bUseElectrodeShiftMode);
}


//*************************************************************************************************************

QSharedPointer<IPlugin> BrainAMP::clone() const
{
    QSharedPointer<BrainAMP> pBrainAMPClone(new BrainAMP());
    return pBrainAMPClone;
}


//*************************************************************************************************************

void BrainAMP::init()
{
    m_pRMTSA_BrainAMP = PluginOutputData<RealTimeMultiSampleArray>::create(this, "BrainAMP", "EEG output data");

    m_outputConnectors.append(m_pRMTSA_BrainAMP);

    //default values used by the setupGUI class must be set here

    QSettings settings;
    m_iSamplingFreq = settings.value(QString("BRAINAMP/sFreq"), 1000).toInt();
    m_iSamplesPerBlock = settings.value(QString("BRAINAMP/samplesPerBlock"), 1000).toInt();
    m_bWriteToFile = false;
    m_bIsRunning = false;
    m_bCheckImpedances = false;

    QDate date;
    m_sOutputFilePath = settings.value(QString("BRAINAMP/outputFilePath"), QString("%1Sequence_01/Subject_01/%2_%3_%4_EEG_001_raw.fif").arg(m_qStringResourcePath).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day())).toString();

    m_sElcFilePath = settings.value(QString("BRAINAMP/elcFilePath"), QString("./Resources/3DLayouts/standard_waveguard64_duke.elc")).toString();

    m_sCardinalFilePath = settings.value(QString("BRAINAMP/cardinalFilePath"), QString("")).toString();

    m_dLPAShift = settings.value(QString("BRAINAMP/LPAShift"), 0.0).toFloat();
    m_dRPAShift = settings.value(QString("BRAINAMP/RPAShift"), 0.0).toFloat();
    m_dNasionShift = settings.value(QString("BRAINAMP/NasionShift"), 0.0).toFloat();
    m_sLPA = settings.value(QString("BRAINAMP/LPAElectrode"), QString("")).toString();
    m_sRPA = settings.value(QString("BRAINAMP/RPAElectrode"), QString("")).toString();
    m_sNasion = settings.value(QString("BRAINAMP/NasionElectrode"), QString("")).toString();
    m_bUseTrackedCardinalMode = settings.value(QString("BRAINAMP/useTrackedCardinalsMode"), true).toBool();
    m_bUseElectrodeShiftMode = settings.value(QString("BRAINAMP/useElectrodeshiftMode"), false).toBool();

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());
}


//*************************************************************************************************************

void BrainAMP::unload()
{

}


//*************************************************************************************************************

void BrainAMP::setUpFiffInfo()
{
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    m_pFiffInfo->nchan = 33;
    m_pFiffInfo->sfreq = m_iSamplingFreq;
    m_pFiffInfo->highpass = (float)0.001;
    m_pFiffInfo->lowpass = m_iSamplingFreq/2;

    //
    //Set up the channel info
    //
    QStringList QSLChNames;
    m_pFiffInfo->chs.clear();

    for(int i = 0; i < m_pFiffInfo->nchan; ++i)
    {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

        //EEG Channels
        if(i <= m_pFiffInfo->nchan-2)
        {
            //Set channel name
            sChType = QString("EEG ");
            if(i<10) {
                sChType.append("00");
            }

            if(i>=10 && i<100) {
                sChType.append("0");
            }

            fChInfo.ch_name = sChType.append(sChType.number(i));

            //Set channel type
            fChInfo.kind = FIFFV_EEG_CH;

            //Set logno
            fChInfo.logNo = i;

            //Set coord frame
            fChInfo.coord_frame = FIFFV_COORD_HEAD;

            //Set unit
            fChInfo.unit = FIFF_UNIT_V;
            fChInfo.unit_mul = 0;

            //Set EEG electrode location - Convert from mm to m
            fChInfo.eeg_loc(0,0) = 0;
            fChInfo.eeg_loc(1,0) = 0;
            fChInfo.eeg_loc(2,0) = 0;

            //Set EEG electrode direction - Convert from mm to m
            fChInfo.eeg_loc(0,1) = 0;
            fChInfo.eeg_loc(1,1) = 0;
            fChInfo.eeg_loc(2,1) = 0;

            //Also write the eeg electrode locations into the meg loc variable (mne_ex_read_raw() matlab function wants this)
            fChInfo.chpos.r0(0) = 0;
            fChInfo.chpos.r0(1) = 0;
            fChInfo.chpos.r0(2) = 0;

            fChInfo.chpos.ex(0) = 1;
            fChInfo.chpos.ex(1) = 0;
            fChInfo.chpos.ex(2) = 0;

            fChInfo.chpos.ey(0) = 0;
            fChInfo.chpos.ey(1) = 1;
            fChInfo.chpos.ey(2) = 0;

            fChInfo.chpos.ez(0) = 0;
            fChInfo.chpos.ez(1) = 0;
            fChInfo.chpos.ez(2) = 1;
        }

        //Digital input channel
        if(i == m_pFiffInfo->nchan-1)
        {
            //Set channel type
            fChInfo.kind = FIFFV_STIM_CH;

            sChType = QString("STIM");
            fChInfo.ch_name = sChType;
        }

        QSLChNames << sChType;

        m_pFiffInfo->chs.append(fChInfo);
    }

    //Set channel names in fiff_info_base
    m_pFiffInfo->ch_names = QSLChNames;

    //
    //Set head projection
    //
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}


//*************************************************************************************************************

bool BrainAMP::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()) {
        QThread::wait();
    }

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_BrainAMP->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRMTSA_BrainAMP->data()->setMultiArraySize(1);
    m_pRMTSA_BrainAMP->data()->setSamplingRate(m_iSamplingFreq);

    //Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_pFiffInfo->nchan, m_iSamplesPerBlock));
    m_qListReceivedSamples.clear();

    m_pBrainAMPProducer->start(m_iSamplesPerBlock,
                       m_iSamplingFreq,
                       m_sOutputFilePath,
                       m_bCheckImpedances);

    if(m_pBrainAMPProducer->isRunning())
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "BrainAMP::start() - BrainAMPProducer thread could not be started." << endl;
        return false;
    }
}


//*************************************************************************************************************

bool BrainAMP::stop()
{
    //Stop the producer thread first
    m_pBrainAMPProducer->stop();

    //Wait until this thread (BrainAMP) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer_In->releaseFromPop();

    m_pRawMatrixBuffer_In->clear();

    m_pRMTSA_BrainAMP->data()->clear();

    m_qListReceivedSamples.clear();

    return true;
}


//*************************************************************************************************************

void BrainAMP::setSampleData(MatrixXd &matRawBuffer)
{
    m_mutex.lock();
    m_qListReceivedSamples.append(matRawBuffer);
    m_mutex.unlock();
}


//*************************************************************************************************************

IPlugin::PluginType BrainAMP::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString BrainAMP::getName() const
{
    return "BrainAMP EEG";
}


//*************************************************************************************************************

QWidget* BrainAMP::setupWidget()
{
    BrainAMPSetupWidget* widget = new BrainAMPSetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initGui();

    return widget;
}


//*************************************************************************************************************

void BrainAMP::onUpdateCardinalPoints(const QString& sLPA, double dLPA, const QString& sRPA, double dRPA, const QString& sNasion, double dNasion)
{
    m_dLPAShift = dLPA;
    m_dRPAShift = dRPA;
    m_dNasionShift = dNasion;

    m_sLPA = sLPA;
    m_sRPA = sRPA;
    m_sNasion = sNasion;
}


//*************************************************************************************************************

void BrainAMP::run()
{
    while(m_bIsRunning)
    {
        if(m_pBrainAMPProducer->isRunning())
        {
            m_mutex.lock();

            if(m_qListReceivedSamples.isEmpty() == false)
            {
                MatrixXd matValue;
                matValue = m_qListReceivedSamples.first();
                m_qListReceivedSamples.removeFirst();

                //Write raw data to fif file
                if(m_bWriteToFile) {
                    m_pOutfid->write_raw_buffer(matValue, m_cals);
                }

                //emit values to real time multi sample array
                //qDebug()<<"BrainAMP::run() - mat size"<<matValue.rows()<<"x"<<matValue.cols();
                //std::cout << "BrainAMP::run() - matValue.block(10,10)" << matValue.block(0,0,10,10) << std::endl;
                m_pRMTSA_BrainAMP->data()->setValue(matValue);
            }

            m_mutex.unlock();            
        }
    }

    //Close the fif output stream
    if(m_bWriteToFile)
    {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    }

    //std::cout<<"EXITING - BrainAMP::run()"<<std::endl;
}


//*************************************************************************************************************

void BrainAMP::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pBrainAMPSetupProjectWidget == Q_NULLPTR) {
        m_pBrainAMPSetupProjectWidget = QSharedPointer<BrainAMPSetupProjectWidget>(new BrainAMPSetupProjectWidget(this));

        connect(m_pBrainAMPSetupProjectWidget.data(), &BrainAMPSetupProjectWidget::cardinalPointsChanged,
                this, &BrainAMP::onUpdateCardinalPoints);
    }

    if(!m_pBrainAMPSetupProjectWidget->isVisible())
    {
        m_pBrainAMPSetupProjectWidget->setWindowTitle("BrainAMP EEG Connector - Setup project");
        m_pBrainAMPSetupProjectWidget->show();
        m_pBrainAMPSetupProjectWidget->raise();
    }
}


//*************************************************************************************************************

void BrainAMP::showStartRecording()
{
    //Setup writing to file
    if(m_bWriteToFile)
    {
        m_pOutfid->finish_writing_raw();
        m_bWriteToFile = false;
        m_pTimerRecordingChange->stop();
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
    }
    else
    {
        if(!m_bIsRunning)
        {
            QMessageBox msgBox;
            msgBox.setText("Start data acquisition first!");
            msgBox.exec();
            return;
        }

        if(!m_pFiffInfo)
        {
            QMessageBox msgBox;
            msgBox.setText("FiffInfo missing!");
            msgBox.exec();
            return;
        }

        //Initiate the stream for writing to the fif file
        m_fileOut.setFileName(m_sOutputFilePath);
        if(m_fileOut.exists())
        {
            QMessageBox msgBox;
            msgBox.setText("The file you want to write already exists.");
            msgBox.setInformativeText("Do you want to overwrite this file?");
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            int ret = msgBox.exec();
            if(ret == QMessageBox::No)
                return;
        }

        // Check if path exists -> otherwise create it
        QStringList list = m_sOutputFilePath.split("/");
        list.removeLast(); // remove file name
        QString fileDir = list.join("/");

        if(!dirExists(fileDir.toStdString()))
        {
            QDir dir;
            dir.mkpath(fileDir);
        }

        m_pOutfid = Fiff::start_writing_raw(m_fileOut, *m_pFiffInfo, m_cals);
        fiff_int_t first = 0;
        m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);

        m_bWriteToFile = true;

        m_pTimerRecordingChange = QSharedPointer<QTimer>(new QTimer);
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &BrainAMP::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}


//*************************************************************************************************************

void BrainAMP::changeRecordingButton()
{
    if(m_iBlinkStatus == 0)
    {
        m_pActionStartRecording->setIcon(QIcon(":/images/record.png"));
        m_iBlinkStatus = 1;
    }
    else
    {
        m_pActionStartRecording->setIcon(QIcon(":/images/record_active.png"));
        m_iBlinkStatus = 0;
    }
}


//*************************************************************************************************************

bool BrainAMP::dirExists(const std::string& dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES)
        return false;  //something is wrong with your path!

    if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return true;   // this is a directory!

    return false;    // this is not a directory!
}


