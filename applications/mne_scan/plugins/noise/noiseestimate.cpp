//=============================================================================================================
/**
* @file     noiseestimate.cpp
* @author   Limin Sun <liminsun@nmr.mgh.harvard.edu>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Limin Sun, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the NoiseEstimate class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noiseestimate.h"
#include "FormFiles/noiseestimatesetupwidget.h"

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QDebug>

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NOISEESTIMATEPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseEstimate::NoiseEstimate()
: m_bIsRunning(false)
, m_bProcessData(false)
, m_pRTMSAInput(NULL)
, m_pFSOutput(NULL)
, m_pBuffer(CircularMatrixBuffer<double>::SPtr())
, m_Fs(600)
, m_iFFTlength(16384)
, m_DataLen(6)
, m_x_scale_type(0)
{
}


//*************************************************************************************************************

NoiseEstimate::~NoiseEstimate()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> NoiseEstimate::clone() const
{
    QSharedPointer<NoiseEstimate> pNoiseEstimateClone(new NoiseEstimate);
    return pNoiseEstimateClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void NoiseEstimate::init()
{    
    //
    // Load Settings
    //
    QSettings settings;
    m_iFFTlength = settings.value(QString("Plugin/%1/FFTLength").arg(this->getName()), 16384).toInt();
    m_DataLen = settings.value(QString("Plugin/%1/DataLen").arg(this->getName()), 6).toInt();
    m_x_scale_type = settings.value(QString("Plugin/%1/ScaleType").arg(this->getName()), 0).toInt();

    // Input
    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "Noise Estimatge In", "Noise Estimate input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &NoiseEstimate::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pFSOutput = PluginOutputData<RealTimeSpectrum>::create(this, "Noise Estimate Out", "Noise Estimate output data");
    m_pFSOutput->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pFSOutput);

    //init channels when fiff info is available
    connect(this, &NoiseEstimate::fiffInfoAvailable, this, &NoiseEstimate::initConnector);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pBuffer.isNull())
        m_pBuffer = CircularMatrixBuffer<double>::SPtr();

}


//*************************************************************************************************************

void NoiseEstimate::unload()
{
    //
    // Store Settings
    //
    QSettings settings;
    settings.setValue(QString("Plugin/%1/FFTLength").arg(this->getName()), m_iFFTlength);
    settings.setValue(QString("Plugin/%1/DataLen").arg(this->getName()), m_DataLen);
    settings.setValue(QString("Plugin/%1/ScaleType").arg(this->getName()), m_x_scale_type);
}


//*************************************************************************************************************

void NoiseEstimate::initConnector()
{
    /* init Connector is called after NoiseEstimate::start */
    qDebug() << "void NoiseEstimate::initConnector()";
    if(m_pFiffInfo){
        // pass fiff info
        m_pFSOutput->data()->initFromFiffInfo(m_pFiffInfo);

    }
}


//*************************************************************************************************************

bool NoiseEstimate::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    m_qMutex.lock();
    m_bIsRunning = true;
    m_qMutex.unlock();

    // Start threads
    QThread::start();
    qDebug()<<"NoiseEstimate Thread is started.";
    return true;
}


//*************************************************************************************************************

bool NoiseEstimate::stop()
{


    //Wait until this thread is stopped
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();

    if(m_bProcessData)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pBuffer->releaseFromPop();
        m_pBuffer->releaseFromPush();

//        m_pBuffer->clear();
//        m_pNEOutput->data()->clear();
    }

    // Stop filling buffers with data from the inputs
    m_bProcessData = false;

    qDebug()<<"NoiseEstimate Thread is stopped.";

//    if(m_pRtNoise && m_pRtNoise->isRunning())
//        m_pRtNoise->stop();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType NoiseEstimate::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString NoiseEstimate::getName() const
{
    return "Spectrum";
}


//*************************************************************************************************************

QWidget* NoiseEstimate::setupWidget()
{
    NoiseEstimateSetupWidget* setupWidget = new NoiseEstimateSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    connect(this,&NoiseEstimate::SetNoisePara,setupWidget,&NoiseEstimateSetupWidget::init);

    return setupWidget;

}


//*************************************************************************************************************

void NoiseEstimate::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA)
    {
        //Check if buffer initialized

        m_qMutex.lock();
        if(!m_pBuffer)
        {
            m_pBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(8, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo)
        {
            m_pFiffInfo = pRTMSA->info();
            emit fiffInfoAvailable();
        }
        m_qMutex.unlock();

        if(m_bProcessData)
        {
            MatrixXd t_mat;

            for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i)
            {
                t_mat = pRTMSA->getMultiSampleArray()[i];
                m_pBuffer->push(&t_mat);
            }
        }
    }
}

//*************************************************************************************************************

void NoiseEstimate::appendNoiseSpectrum(MatrixXd t_send)
{ 
    qDebug()<<"Spectrum"<<t_send(0,1)<<t_send(0,2)<<t_send(0,3);
    m_qMutex.lock();
    m_qVecSpecData.push_back(t_send);
    m_qMutex.unlock();
    qDebug()<<"---------------------------------appendNoiseSpectrum--------------------------------";
}


//*************************************************************************************************************

void NoiseEstimate::run()
{
    //
    // Read Fiff Info
    //
    bool waitForFiffInfo = true;
    while(waitForFiffInfo)
    {
        m_qMutex.lock();
        if(m_pFiffInfo)
            waitForFiffInfo = false;
        m_qMutex.unlock();
        msleep(10);// Wait for fiff Info
    }

    // Set up the x-axis scale type
    m_pFSOutput->data()->initScaleType(m_x_scale_type);
    qDebug()<< "Scale Type [0-normal; 1-log]:" << m_x_scale_type;


    // Init Real-Time Noise Spectrum estimator
    //
    // calculate the segments according to the requested data length
    // here 500 is the number of samples for a block specified in babyMEG plugin
    m_Fs = m_pFiffInfo->sfreq;
    int segments =  (qint32) ((m_DataLen * m_pFiffInfo->sfreq)/m_pBuffer->cols());

    qDebug()<<"+++++++++++segments :"<<segments<< "m_DataLen"<<m_DataLen<<"m_Fs"<<m_Fs<<"++++++++++++++++++++++++";

    m_pRtNoise = RtNoise::SPtr(new RtNoise(m_iFFTlength, m_pFiffInfo, segments));
    connect(m_pRtNoise.data(), &RtNoise::SpecCalculated, this, &NoiseEstimate::appendNoiseSpectrum);

    // Start Spectrum estimation

    m_pRtNoise->start();

    m_qMutex.lock();
    m_bProcessData = true;
    m_qMutex.unlock();

    while (m_bIsRunning)
    {

        if(m_bProcessData)
        {
            /* Dispatch the inputs */
            MatrixXd t_mat = m_pBuffer->pop();

            //ToDo: Implement your algorithm here
            m_pRtNoise->append(t_mat);

           if(m_qVecSpecData.size() > 0)
           {
               m_qMutex.lock();
               qDebug()<<"%%%%%%%%%%%%%%%% send spectrum for display %%%%%%%%%%%%%%%%%%%";
                //send spectrum to the output data
               m_pFSOutput->data()->setValue(m_qVecSpecData[0]);
               m_qVecSpecData.pop_front();
               m_qMutex.unlock();

            }
        }//m_bProcessData
    }//m_bIsRunning
    qDebug()<<"noise estimation [Run] is done!";
    m_pRtNoise->stop();
//    delete m_pRtNoise;
}

