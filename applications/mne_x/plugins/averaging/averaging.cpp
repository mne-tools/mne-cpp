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
* @brief    Contains the implementation of the Averaging class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averaging.h"
#include "FormFiles/averagingsetupwidget.h"
#include "FormFiles/averagingsettingswidget.h"

#include <iostream>


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
, m_iPreStimSamples(400)
, m_iPostStimSamples(750)
, m_iNumAverages(10)
, m_iStimChan(0)
, m_pAveragingWidget(AveragingSettingsWidget::SPtr())
, m_pActionShowAdjustment(Q_NULLPTR)
{
    m_pActionShowAdjustment = new QAction(QIcon(":/images/averaging.png"), tr("Averaging Adjustments"),this);
//    m_pActionSetupProject->setShortcut(tr("F12"));
    m_pActionShowAdjustment->setStatusTip(tr("Averaging Adjustments"));
    connect(m_pActionShowAdjustment, &QAction::triggered, this, &Averaging::showAveragingWidget);
    addPluginAction(m_pActionShowAdjustment);

    m_pActionShowAdjustment->setVisible(false);
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

void Averaging::changeNumAverages(qint32 numAve)
{
    m_iNumAverages = numAve;
    if(m_pRtAve)
        m_pRtAve->setAverages(numAve);
}


//*************************************************************************************************************

void Averaging::initConnector()
{
    if(m_pFiffInfo)
    {
        m_qListModalities.clear();
        bool hasMag = false;
        bool hasGrad = false;
        bool hasEEG = false;
        bool hasEOG = false;
        for(qint32 i = 0; i < m_pFiffInfo->nchan; ++i)
        {
            if(m_pFiffInfo->chs[i].kind == FIFFV_MEG_CH)
            {
                if(!hasMag &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T)
                    hasMag = true;
                else if(!hasGrad &&  m_pFiffInfo->chs[i].unit == FIFF_UNIT_T_M)
                    hasGrad = true;
            }
            else if(!hasEEG && m_pFiffInfo->chs[i].kind == FIFFV_EEG_CH)
                hasEEG = true;
            else if(!hasEOG && m_pFiffInfo->chs[i].kind == FIFFV_EOG_CH)
                hasEOG = true;
        }
        if(hasMag)
            m_qListModalities.append(QPair<QString,bool>("MAG",true));
        if(hasGrad)
            m_qListModalities.append(QPair<QString,bool>("GRAD",true));
        if(hasEEG)
            m_qListModalities.append(QPair<QString,bool>("EEG",true));
        if(hasEOG)
            m_qListModalities.append(QPair<QString,bool>("EOG",true));
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

void Averaging::changeStimChannel(qint32 index)
{
    Q_UNUSED(index)
    m_iStimChan = m_pAveragingWidget->m_pComboBoxChSelection->currentData().toInt();
//    qDebug() << "Averaging::changeStimChannel(qint32 index)" << m_pAveragingWidget->m_pComboBoxChSelection->currentData().toInt();
}

//*************************************************************************************************************

void Averaging::changePreStim(qint32 samples)
{
    m_iPreStimSamples = samples;
    if(m_pRtAve)
        m_pRtAve->setPreStim(m_iPreStimSamples);

}


//*************************************************************************************************************

void Averaging::changePostStim(qint32 samples)
{
    m_iPostStimSamples = samples;
    if(m_pRtAve)
        m_pRtAve->setPostStim(m_iPostStimSamples);
}


//*************************************************************************************************************

QWidget* Averaging::setupWidget()
{
    AveragingSetupWidget* setupWidget = new AveragingSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Averaging::showAveragingWidget()
{
    m_pAveragingWidget = AveragingSettingsWidget::SPtr(new AveragingSettingsWidget(this));
    m_pAveragingWidget->show();
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

void Averaging::appendEvoked(FiffEvoked::SPtr p_pEvoked)
{
//    qDebug() << "void Averaging::appendEvoked";// << p_pEvoked->comment;
//    qDebug() << p_pEvoked->comment;
    QString t_sStimulusChannel = m_pFiffInfo->chs[m_qListStimChs[m_iStimChan]].ch_name;

    if(p_pEvoked->comment == t_sStimulusChannel)
    {
        qDebug()<< "append" << p_pEvoked->comment << "=" << t_sStimulusChannel;
        mutex.lock();
        m_qVecEvokedData.push_back(p_pEvoked);
        mutex.unlock();
        qDebug() << "append after" << m_qVecEvokedData.size();
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

    m_pActionShowAdjustment->setVisible(true);

    for(qint32 i = 0; i < m_pFiffInfo->chs.size(); ++i)
    {
        if(m_pFiffInfo->chs[i].kind == FIFFV_STIM_CH)
        {
            qDebug() << "Stim" << i << "Name" << m_pFiffInfo->chs[i].ch_name;
            m_qListStimChs.append(i);
        }
    }

    m_bProcessData = true;

    //
    // Init Real-Time average
    //
    m_pRtAve = RtAve::SPtr(new RtAve(m_iNumAverages, m_iPreStimSamples, m_iPostStimSamples, m_pFiffInfo));
    connect(m_pRtAve.data(), &RtAve::evokedStim, this, &Averaging::appendEvoked);

    m_pRtAve->start();

    while (m_bIsRunning)
    {
        if(m_bProcessData)
        {
            /* Dispatch the inputs */
            MatrixXd rawSegment = m_pAveragingBuffer->pop();

            m_pRtAve->append(rawSegment);

            mutex.lock();
            if(m_qVecEvokedData.size() > 0)
            {
                FiffEvoked t_fiffEvoked = *m_qVecEvokedData[0].data();

                m_pAveragingOutput->data()->setValue(t_fiffEvoked);

                m_qVecEvokedData.pop_front();

            }
            mutex.unlock();

        }
    }


    m_pActionShowAdjustment->setVisible(false);

    m_pRtAve->stop();
}

