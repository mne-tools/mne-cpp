//=============================================================================================================
/**
 * @file     rthpi.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Limin Sun <limin.sun@childrens.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     June, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Christoph Dinh, Limin Sun, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the RtHpi class.
 *
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================
#include <Eigen/Dense>
#include <iostream>
#include "rthpi.h"
#include "FormFiles/rthpisetupwidget.h"
#include <math.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>      // std::ifstream

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

using namespace RTHPIPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtHpi::RtHpi()
: m_bIsRunning(false)
, m_bProcessData(false)
, m_pRTMSAInput(NULL)
, m_pRTMSAOutput(NULL)
, m_pRtHpiBuffer(CircularMatrixBuffer<double>::SPtr())
{
}


//*************************************************************************************************************

RtHpi::~RtHpi()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> RtHpi::clone() const
{
    QSharedPointer<RtHpi> pRtHpiClone(new RtHpi);
    return pRtHpiClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Creating required display instances and set configurations
//=============================================================================================================

void RtHpi::init()
{
    // Input
    m_pRTMSAInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "Rt HPI In", "RT HPI input data");
    connect(m_pRTMSAInput.data(), &PluginInputConnector::notify, this, &RtHpi::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSAInput);

    // Output
    m_pRTMSAOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "Rt HPI Out", "RT HPI output data");
    m_pRTMSAOutput->data()->setName(this->getName());//Provide name to auto store widget settings
    m_outputConnectors.append(m_pRTMSAOutput);

//    m_pRTMSAOutput->data()->setMultiArraySize(100);
//    m_pRTMSAOutput->data()->setVisibility(true);

    //init channels when fiff info is available
    connect(this, &RtHpi::fiffInfoAvailable, this, &RtHpi::initConnector);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pRtHpiBuffer.isNull())
        m_pRtHpiBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void RtHpi::unload()
{

}


//*************************************************************************************************************

void RtHpi::initConnector()
{
    qDebug() << "void RtHpi::initConnector()";
    if(m_pFiffInfo)
        m_pRTMSAOutput->data()->initFromFiffInfo(m_pFiffInfo);
}


//*************************************************************************************************************

bool RtHpi::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    m_bIsRunning = true;

    // Start threads
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool RtHpi::stop()
{
    //Wait until this thread is stopped
    m_qMutex.lock();
    m_bIsRunning = false;
    m_qMutex.unlock();


    if(m_bProcessData)
    {
        //In case the semaphore blocks the thread -> Release the QSemaphore and let it exit from the pop function (acquire statement)
        m_pRtHpiBuffer->releaseFromPop();
        m_pRtHpiBuffer->releaseFromPush();

//        m_pRtHpiBuffer->clear();

//        m_pRTMSAOutput->data()->clear();
    }

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType RtHpi::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString RtHpi::getName() const
{
    return "RtHpi";
}


//*************************************************************************************************************

QWidget* RtHpi::setupWidget()
{
    RtHpiSetupWidget* setupWidget = new RtHpiSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void RtHpi::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA)
    {
        m_qMutex.lock();
        //Check if buffer initialized
        if(!m_pRtHpiBuffer)
            m_pRtHpiBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(8, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));

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
                m_pRtHpiBuffer->push(&t_mat);
            }
        }
    }
}



//*************************************************************************************************************

void RtHpi::run()
{
    //
    // Read Fiff Info and wait if absent
    //
    bool waitForFiffInfo = true;
    while(waitForFiffInfo)
    {
        m_qMutex.lock();

        if(m_pFiffInfo) waitForFiffInfo = false;

        m_qMutex.unlock();
        msleep(10);// Wait for fiff Info
    }

    m_qMutex.lock();
    m_bProcessData = true;
    m_qMutex.unlock();

    qDebug()<<"+++++++++++ Start Real time HPI thread +++++++++++++++++++";

    m_pRtHPIS = RtHPIS::SPtr(new RtHPIS(m_pFiffInfo));

    while (m_bIsRunning) {
        if(m_bProcessData) {
            MatrixXd t_mat = m_pRtHpiBuffer->pop();
            m_pRtHPIS->append(t_mat);
        }
        //msleep(1);
    }
    qDebug()<<"HPI estimation [Run] is done!";
}

