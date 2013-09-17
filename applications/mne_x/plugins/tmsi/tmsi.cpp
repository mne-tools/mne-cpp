//=============================================================================================================
/**
* @file     tmsi.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     September, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the TMSI class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "tmsi.h"
#include "tmsiproducer.h"

#include "FormFiles/tmsisetupwidget.h"

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QtCore/QtPlugin>
#include <QtCore/QTextStream>
#include <QtCore/QFile>

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TMSIPlugin;
using namespace XMEASLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TMSI::TMSI()
: m_pRTSA_TMSI(0)
, m_iSamplingFreq(512)
, m_iNumberOfChannels(32)
, m_iSamplesPerBlock(32)
, m_iBufferSize(-1)
, m_pRawMatrixBuffer_In(0)
, m_pTMSIProducer(new TMSIProducer(this))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/tmsi/")
{
}


//*************************************************************************************************************

TMSI::~TMSI()
{
    std::cout << "TMSI::~TMSI()" << std::endl;
}


//*************************************************************************************************************

QSharedPointer<IPlugin> TMSI::clone() const
{
    QSharedPointer<TMSI> pTMSIClone(new TMSI());
    return pTMSIClone;
}


//*************************************************************************************************************
//=============================================================================================================
// Create measurement instances and config them
//=============================================================================================================

void TMSI::init()
{
    m_pRTSA_TMSI = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");
    m_outputConnectors.append(m_pRTSA_TMSI);
}


//*************************************************************************************************************

bool TMSI::start()
{
    // Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(m_iSamplesPerBlock,m_iNumberOfChannels,m_iBufferSize));

    // Start threads
    m_pTMSIProducer->start();

    QThread::start();

    return true;
}


//*************************************************************************************************************

bool TMSI::stop()
{
    // Stop threads
    m_pTMSIProducer->stop();
    QThread::terminate();
    QThread::wait();

    //Clear Buffers
    m_pRawMatrixBuffer_In->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType TMSI::getType() const
{
    return _ISensor;
}


//*************************************************************************************************************

QString TMSI::getName() const
{
    return "TMSI EEG";
}


//*************************************************************************************************************

QWidget* TMSI::setupWidget()
{
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initSamplingProperties();

    return widget;
}


//*************************************************************************************************************

void TMSI::run()
{
    MatrixXf matValue;

    while(true)
    {
        //pop matrix
        matValue = m_pRawMatrixBuffer_In->pop();
        //        std::cout << "matValue " << matValue.block(0,0,1,10) << std::endl;

        //emit values
        for(qint32 i = 0; i < matValue.cols(); ++i)
            m_pRTSA_TMSI->data()->setValue(matValue.col(i).cast<double>());
    }
}
