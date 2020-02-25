//=============================================================================================================
/**
 * @file     gusbamp.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Contains the implementation of the GUSBAmp class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "gusbamp.h"
#include "gusbampproducer.h"   


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>


//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace GUSBAMPPLUGIN;
using namespace IOBUFFER;
using namespace std;


//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmp::GUSBAmp()
: m_pRTMSA_GUSBAmp(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/resources/mne_scan/plugins/gusbamp/")
, m_pRawMatrixBuffer_In(0)
, m_pGUSBAmpProducer(new GUSBAmpProducer(this))
, m_iNumberOfChannels(0)
, m_iSamplesPerBlock(0)
, m_iSampleRate(128)
, m_bWriteToFile(false)
{
    m_viChannelsToAcquire = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    m_viSizeOfSampleMatrix.resize(2,0);

    m_vSerials.resize(1);
    m_vSerials[0]= "UB-2015.05.16";

    // Create record file option action bar item/button
    m_pActionSetupProject = new QAction(QIcon(":/images/database.png"), tr("Setup project"), this);
    m_pActionSetupProject->setStatusTip(tr("Setup project"));
    connect(m_pActionSetupProject, &QAction::triggered, this, &GUSBAmp::showSetupProjectDialog);
    addPluginAction(m_pActionSetupProject);

    // Create start recordin action bar item/button
    m_pActionStartRecording = new QAction(QIcon(":/images/record.png"), tr("Start recording data to fif file"), this);
    m_pActionStartRecording->setStatusTip(tr("Start recording data to fif file"));
    connect(m_pActionStartRecording, &QAction::triggered, this, &GUSBAmp::showStartRecording);
    addPluginAction(m_pActionStartRecording);

}


//=============================================================================================================

GUSBAmp::~GUSBAmp()
{
    //std::cout << "GUSBAmp::~GUSBAmp() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning()){
        this->stop();
    }
}


//=============================================================================================================

void GUSBAmp::setUpFiffInfo()
{
    // Only works for ANT Neuro Waveguard Duke caps
    //
    //Clear old fiff info data
    //
    m_pFiffInfo->clear();

    //
    //Set number of channels, sampling frequency and high/-lowpass
    //
    m_pFiffInfo->nchan = m_iNumberOfChannels;
    m_pFiffInfo->sfreq = m_iSampleRate;
    m_pFiffInfo->highpass = (float)0.001;
    m_pFiffInfo->lowpass = m_iSampleRate/2;

    int numberEEGCh = m_iNumberOfChannels;

    //
    //Set up the channel info
    //
    QStringList QSLChNames;
    m_pFiffInfo->chs.clear();

    for(int i=0; i<m_iNumberOfChannels; i++)
    {
        //Create information for each channel
        QString sChType;
        FiffChInfo fChInfo;

        //EEG Channels
        if(i<=numberEEGCh-1)
        {
            //Set channel name
            //fChInfo.ch_name = elcChannelNames.at(i);
            sChType = QString("EEG ");
            if(i<10){
                sChType.append("00");
            }

            if(i>=10 && i<100){
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

            //cout<<i<<endl<<fChInfo.eeg_loc<<endl;
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


//=============================================================================================================

QSharedPointer<IPlugin> GUSBAmp::clone() const
{
    QSharedPointer<GUSBAmp> pGUSBAmpClone(new GUSBAmp());
    return pGUSBAmpClone;
}


//=============================================================================================================

void GUSBAmp::init()
{
    m_iSplitFileSizeMs = 10;
    m_iSplitCount = 0;
    m_bSplitFile = false;

    QDate date;
    m_sOutputFilePath = QString ("%1Sequence_01/Subject_01/%2_%3_%4_EEG_001_raw.fif").arg(m_qStringResourcePath).arg(date.currentDate().year()).arg(date.currentDate().month()).arg(date.currentDate().day());

    m_pRTMSA_GUSBAmp = PluginOutputData<RealTimeMultiSampleArray>::create(this, "GUSBAmp", "EEG output data");

    m_outputConnectors.append(m_pRTMSA_GUSBAmp);

    m_pFiffInfo = QSharedPointer<FiffInfo>(new FiffInfo());

    m_bIsRunning = false;
}


//=============================================================================================================

void GUSBAmp::unload()
{

}


//=============================================================================================================

bool GUSBAmp::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning()){
        QThread::wait();
    }

    //get the values from the GUI and start GUSBAmpProducer
    m_pGUSBAmpProducer->start(m_vSerials, m_viChannelsToAcquire, m_iSampleRate);


    //after device was started: ask for size of SampleMatrix to set the buffer matrix (bevor setUpFiffInfo() is started)
    m_viSizeOfSampleMatrix = m_pGUSBAmpProducer->getSizeOfSampleMatrix();
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_viSizeOfSampleMatrix[0], m_viSizeOfSampleMatrix[1]));

    //set the parameters for number of channels (rows of matrix) and samples (columns of matrix)
    m_iNumberOfChannels = m_viSizeOfSampleMatrix[0];
    m_iSamplesPerBlock  = m_viSizeOfSampleMatrix[1];

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RTMSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRTMSA_GUSBAmp->data()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_GUSBAmp->data()->setMultiArraySize(1);
    m_pRTMSA_GUSBAmp->data()->setSamplingRate(m_iSampleRate);

    //start the thread for ring buffer
    if(m_pGUSBAmpProducer->isRunning())
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "Plugin GUSBAmp - ERROR - GUSBAmpProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (GUSBAmpSDK.dll / GUSBAmpSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}


//=============================================================================================================

bool GUSBAmp::stop()
{
    //Stop the producer thread first
    m_pGUSBAmpProducer->stop();

    //Wait until this thread (GUSBAmp) is stopped
    m_bIsRunning = false;

    //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
    m_pRawMatrixBuffer_In->releaseFromPop();

    m_pRawMatrixBuffer_In->clear();

    m_pRTMSA_GUSBAmp->data()->clear();

    return true;
}


//=============================================================================================================

IPlugin::PluginType GUSBAmp::getType() const
{
    return _ISensor;
}


//=============================================================================================================

QString GUSBAmp::getName() const
{
    return "GUSBAmp EEG";
}


//=============================================================================================================

QWidget* GUSBAmp::setupWidget()
{
    GUSBAmpSetupWidget* pWidget = new GUSBAmpSetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    pWidget->initGui();

    return pWidget;
}


//=============================================================================================================

void GUSBAmp::run()
{
    qint32 size = 0;

    //get Matrix from the producer
    while(m_bIsRunning)
    {
        //pop matrix only if the producer thread is running
        if(m_pGUSBAmpProducer->isRunning())
        {
            //qDebug()<<"GUSBAmp is running";
            MatrixXf matValue = m_pRawMatrixBuffer_In->pop();
            MatrixXf matValue_show = matValue/1000000; //matvalue for showing

            for(int i = 0; i < matValue.cols(); i++){
                qDebug() << matValue(0,i);
            }

            //emit values to real time multi sample array
            m_pRTMSA_GUSBAmp->data()->setValue(matValue_show.cast<double>());
            qDebug() << "PUSH!";

            //Write raw data to fif file
            if(m_bWriteToFile)
            {
                m_pOutfid->write_raw_buffer(matValue.cast<double>(), m_cals);
                size += matValue.cols();

                //                qDebug()<<"size"<<size;
                //                qDebug()<<"(m_iSplitFileSizeMs/1000)*m_pFiffInfo->sfreq"<<(double(m_iSplitFileSizeMs)/1000.0)*m_pFiffInfo->sfreq;
                if(size > (double(m_iSplitFileSizeMs)/1000.0)*m_pFiffInfo->sfreq && m_bSplitFile)
                {
                    size = 0;
                    splitRecordingFile();
                }
            }
            else
                size = 0;
        }
    }
}


//=============================================================================================================

void GUSBAmp::splitRecordingFile()
{
    qDebug() << "Split recording file";
    ++m_iSplitCount;
    QString nextFileName = m_sOutputFilePath.remove("_raw.fif");
    nextFileName += QString("-%1_raw.fif").arg(m_iSplitCount);

    /*
     * Write the link to the next file
     */
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
    m_fileOut.setFileName(nextFileName);
    m_pOutfid = Fiff::start_writing_raw(m_fileOut, *m_pFiffInfo, m_cals);
    fiff_int_t first = 0;
    m_pOutfid->write_int(FIFF_FIRST_SAMPLE, &first);
}


//=============================================================================================================

void GUSBAmp::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pGUSBampSetupProjectWidget == NULL){
        m_pGUSBampSetupProjectWidget = QSharedPointer<GUSBAmpSetupProjectWidget>(new GUSBAmpSetupProjectWidget(this));
    }
    if(!m_pGUSBampSetupProjectWidget->isVisible())
    {
        m_pGUSBampSetupProjectWidget->setWindowTitle("GUSBAmp EEG Connector - Setup project");
        m_pGUSBampSetupProjectWidget->initGui();
        m_pGUSBampSetupProjectWidget->show();
        m_pGUSBampSetupProjectWidget->raise();
    }
}


//=============================================================================================================

void GUSBAmp::showStartRecording()
{
    m_iSplitCount = 0;

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
            if(ret == QMessageBox::No){
                return;
            }
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
        connect(m_pTimerRecordingChange.data(), &QTimer::timeout, this, &GUSBAmp::changeRecordingButton);
        m_pTimerRecordingChange->start(500);
    }
}


//=============================================================================================================

void GUSBAmp::changeRecordingButton()
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


//=============================================================================================================

bool GUSBAmp::dirExists(const std::string& dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES){
        return false;  //something is wrong with your path!
    }
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY){
        return true;   // this is a directory!
    }
    return false;    // this is not a directory!
}
