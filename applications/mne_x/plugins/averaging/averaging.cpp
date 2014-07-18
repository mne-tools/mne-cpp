//=============================================================================================================
/**
* @file     averaging.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the implementation of the Averaging class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"
#include "FormFiles/averagingsetupwidget.h"


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

using namespace AveragingPlugin;
using namespace MNEX;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Averaging::Averaging()
: m_pAveragingInput(NULL)
//, m_pAveragingOutput(NULL)
, m_pAveragingBuffer(CircularMatrixBuffer<double>::SPtr())
, m_bIsRunning(false)
, m_bProcessData(false)
, m_pSpinBoxPreStimSamples(NULL)
, m_pSpinBoxPostStimSamples(NULL)
, m_iPreStimSamples(200)
, m_iPostStimSamples(1000)
, m_iDebugNumChannels(-1)
{

//    m_pSpinBoxPreStimSamples = new QSpinBox;
//    m_pSpinBoxPreStimSamples->setMinimum(1);
//    m_pSpinBoxPreStimSamples->setMaximum(10000);
//    m_pSpinBoxPreStimSamples->setSingleStep(1);
//    m_pSpinBoxPreStimSamples->setValue(m_iPreStimSamples);
//    connect(m_pSpinBoxPreStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::preStimChanged);
//    addPluginWidget(m_pSpinBoxPreStimSamples);

//    m_pSpinBoxPostStimSamples = new QSpinBox;
//    m_pSpinBoxPostStimSamples->setMinimum(1);
//    m_pSpinBoxPostStimSamples->setMaximum(10000);
//    m_pSpinBoxPostStimSamples->setSingleStep(1);
//    m_pSpinBoxPostStimSamples->setValue(m_iPostStimSamples);
//    connect(m_pSpinBoxPostStimSamples, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &Averaging::postStimChanged);
//    addPluginWidget(m_pSpinBoxPostStimSamples);

}


//*************************************************************************************************************

Averaging::~Averaging()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Averaging::clone() const
{
    QSharedPointer<Averaging> pAveragingClone(new Averaging);
    return pAveragingClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void Averaging::init()
{

    // Input
    m_pAveragingInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "AveragingIn", "Averaging input data");
    connect(m_pAveragingInput.data(), &PluginInputConnector::notify, this, &Averaging::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pAveragingInput);

    // Output
    m_pAveragingOutput = PluginOutputData<RealTimeEvoked>::create(this, "AveragingOut", "Averaging Output Data");
    m_outputConnectors.append(m_pAveragingOutput);

    //init channels when fiff info is available
    connect(this, &Averaging::fiffInfoAvailable, this, &Averaging::initConnector);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pAveragingBuffer.isNull())
        m_pAveragingBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void Averaging::initConnector()
{
    if(m_pFiffInfo)
    {
        m_pAveragingOutput->data()->initFromFiffInfo(m_pFiffInfo);
        m_iDebugNumChannels = m_pAveragingOutput->data()->getNumChannels();
    }
}


//*************************************************************************************************************

bool Averaging::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool Averaging::stop()
{
    //Wait until this thread is stopped
    m_bIsRunning = false;

    if(m_bProcessData)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pAveragingBuffer->releaseFromPop();
        m_pAveragingBuffer->releaseFromPush();

        m_pAveragingBuffer->clear();

//        m_pRTMSAOutput->data()->clear();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Averaging::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString Averaging::getName() const
{
    return "Averaging";
}


//*************************************************************************************************************

void Averaging::preStimChanged(qint32 samples)
{
    m_iPreStimSamples = samples;
    emit sampleNumChanged();
}


//*************************************************************************************************************

void Averaging::postStimChanged(qint32 samples)
{
    m_iPostStimSamples = samples;
    emit sampleNumChanged();
}


//*************************************************************************************************************

QWidget* Averaging::setupWidget()
{
    AveragingSetupWidget* setupWidget = new AveragingSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Averaging::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA)
    {
        //Check if buffer initialized
        if(!m_pAveragingBuffer)
            m_pAveragingBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize()));

        //Fiff information
        if(!m_pFiffInfo)
        {
            m_pFiffInfo = pRTMSA->getFiffInfo();
            emit fiffInfoAvailable();
        }


        if(m_bProcessData)
        {
            MatrixXd t_mat(pRTMSA->getNumChannels(), pRTMSA->getMultiArraySize());

            for(qint32 i = 0; i < pRTMSA->getMultiArraySize(); ++i)
                t_mat.col(i) = pRTMSA->getMultiSampleArray()[i];

            m_pAveragingBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************

void Averaging::run()
{
    //
    // Read Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i)
    {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH)
        {
            qDebug() << "Stim" << i << "Name" << m_pFiffInfo->chs[i].ch_name;
            m_qListStimChs.append(i);
        }
    }

    m_bProcessData = true;

    qint32 count = 0;//DEBUG

    while (m_bIsRunning)
    {
        if(m_bProcessData)
        {
            /* Dispatch the inputs */
            MatrixXd t_mat = m_pAveragingBuffer->pop();

            // DEBUG
            if(count > 10)
            {
                if(m_iDebugNumChannels > 0)
                {
                    MatrixXd test = MatrixXd::Random(m_iDebugNumChannels, 4000);

                    m_pAveragingOutput->data()->setValue(test);
                }

                count = 0;
            }
            // DEBUG End
            ++count;
        }
    }
}

