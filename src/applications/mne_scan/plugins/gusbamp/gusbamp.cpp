//=============================================================================================================
/**
 * @file     gusbamp.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
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
#include <fiff/fiff_ch_info.h>

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
using namespace UTILSLIB;
using namespace Eigen;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

GUSBAmp::GUSBAmp()
: m_pRTMSA_GUSBAmp(0)
, m_qStringResourcePath(qApp->applicationDirPath()+"/../resources/mne_scan/plugins/gusbamp/")
, m_pGUSBAmpProducer(new GUSBAmpProducer(this))
, m_iNumberOfChannels(0)
, m_iSamplesPerBlock(0)
, m_iSampleRate(128)
, m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_float>(new CircularBuffer_Matrix_float(8)))
{
    m_viChannelsToAcquire = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};

    m_viSizeOfSampleMatrix.resize(2,0);

    m_vSerials.resize(1);
    m_vSerials[0]= "UB-2015.05.16";
}

//=============================================================================================================

GUSBAmp::~GUSBAmp()
{
    //If the program is closed while the sampling is in process
    if(this->isRunning()){
        this->stop();
    }
}

//=============================================================================================================

void GUSBAmp::setUpFiffInfo()
{
    // Only works for ANT Neuro Waveguard Duke caps
    //Clear old fiff info data
    m_pFiffInfo->clear();

    //Set number of channels, sampling frequency and high/-lowpass
    m_pFiffInfo->nchan = m_iNumberOfChannels;
    m_pFiffInfo->sfreq = m_iSampleRate;
    m_pFiffInfo->highpass = (float)0.001;
    m_pFiffInfo->lowpass = m_iSampleRate/2;

    int numberEEGCh = m_iNumberOfChannels;

    //Set up the channel info
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

    //Set head projection
    m_pFiffInfo->dev_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->dev_head_t.to = FIFFV_COORD_HEAD;
    m_pFiffInfo->ctf_head_t.from = FIFFV_COORD_DEVICE;
    m_pFiffInfo->ctf_head_t.to = FIFFV_COORD_HEAD;
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> GUSBAmp::clone() const
{
    QSharedPointer<GUSBAmp> pGUSBAmpClone(new GUSBAmp());
    return pGUSBAmpClone;
}

//=============================================================================================================

void GUSBAmp::init()
{
    m_pRTMSA_GUSBAmp = PluginOutputData<RealTimeMultiSampleArray>::create(this, "GUSBAmp", "EEG output data");
    m_pRTMSA_GUSBAmp->measurementData()->setName(this->getName());//Provide name to auto store widget settings

    m_outputConnectors.append(m_pRTMSA_GUSBAmp);
}

//=============================================================================================================

void GUSBAmp::unload()
{
}

//=============================================================================================================

bool GUSBAmp::start()
{
    //get the values from the GUI and start GUSBAmpProducer
    m_pGUSBAmpProducer->start(m_vSerials, m_viChannelsToAcquire, m_iSampleRate);

    //after device was started: ask for size of SampleMatrix to set the buffer matrix (bevor setUpFiffInfo() is started)
    m_viSizeOfSampleMatrix = m_pGUSBAmpProducer->getSizeOfSampleMatrix();

    //set the parameters for number of channels (rows of matrix) and samples (columns of matrix)
    m_iNumberOfChannels = m_viSizeOfSampleMatrix[0];
    m_iSamplesPerBlock  = m_viSizeOfSampleMatrix[1];

    //Setup fiff info
    setUpFiffInfo();

    //Set the channel size of the RTMSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRTMSA_GUSBAmp->measurementData()->initFromFiffInfo(m_pFiffInfo);
    m_pRTMSA_GUSBAmp->measurementData()->setMultiArraySize(1);
    m_pRTMSA_GUSBAmp->measurementData()->setSamplingRate(m_iSampleRate);

    //start the thread for ring buffer
    if(m_pGUSBAmpProducer->isRunning())  {
        QThread::start();
        return true;
    } else  {
        qWarning() << "Plugin GUSBAmp - ERROR - GUSBAmpProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (GUSBAmpSDK.dll / GUSBAmpSDK32bit.dll) is not installed in the system32 / SysWOW64 directory" << endl;
        return false;
    }
}

//=============================================================================================================

bool GUSBAmp::stop()
{
    // Stop this (consumer) thread first
    requestInterruption();
    wait(500);

    //Stop the producer thread first
    m_pGUSBAmpProducer->stop();

    // Clear all data in the buffer connected to displays and other plugins
    m_pRTMSA_GUSBAmp->measurementData()->clear();
    m_pCircularBuffer->clear();

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType GUSBAmp::getType() const
{
    return _ISensor;
}

//=============================================================================================================

QString GUSBAmp::getName() const
{
    return "GUSBAmp";
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
    MatrixXf matValue;

    while(!isInterruptionRequested()) {
        //pop matrix only if the producer thread is running
        if(m_pGUSBAmpProducer->isRunning()) {
            //pop matrix
            if(m_pCircularBuffer->pop(matValue)) {
                for(int i = 0; i < matValue.cols(); i++) {
                    qDebug() << matValue(0,i);
                }

                //emit values to real time multi sample array
                m_pRTMSA_GUSBAmp->measurementData()->setValue(matValue.cast<double>()/1000000);
            }
        }
    }
}

//=============================================================================================================

void GUSBAmp::showSetupProjectDialog()
{
    // Open setup project widget
    if(m_pGUSBampSetupProjectWidget == NULL) {
        m_pGUSBampSetupProjectWidget = QSharedPointer<GUSBAmpSetupProjectWidget>(new GUSBAmpSetupProjectWidget(this));
    }
    if(!m_pGUSBampSetupProjectWidget->isVisible()) {
        m_pGUSBampSetupProjectWidget->setWindowTitle("GUSBAmp EEG Connector - Setup project");
        m_pGUSBampSetupProjectWidget->initGui();
        m_pGUSBampSetupProjectWidget->show();
        m_pGUSBampSetupProjectWidget->raise();
    }
}

//=============================================================================================================

bool GUSBAmp::dirExists(const std::string& dirName_in)
{
    DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
    if (ftyp == INVALID_FILE_ATTRIBUTES) {
        return false;  //something is wrong with your path!
    }
    if (ftyp & FILE_ATTRIBUTE_DIRECTORY) {
        return true;   // this is a directory!
    }
    return false;    // this is not a directory!
}

//=============================================================================================================

QString GUSBAmp::getBuildInfo()
{
    return QString(GUSBAMPPLUGIN::buildDateTime()) + QString(" - ")  + QString(GUSBAMPPLUGIN::buildHash());
}
