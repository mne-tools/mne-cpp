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
: m_pRMTSA_TMSI(0)
, m_pRawMatrixBuffer_In(0)
, m_pTMSIProducer(new TMSIProducer(this))
, m_qStringResourcePath(qApp->applicationDirPath()+"/mne_x_plugins/resources/tmsi/")
{
}


//*************************************************************************************************************

TMSI::~TMSI()
{
    //std::cout << "TMSI::~TMSI() " << std::endl;

    //If the program is closed while the sampling is in process
    if(this->isRunning())
        this->stop();
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
    m_pRMTSA_TMSI = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "TMSI", "EEG output data");

    m_outputConnectors.append(m_pRMTSA_TMSI);

    m_iSamplingFreq = 1024;
    m_iNumberOfChannels = 138;
    m_iSamplesPerBlock = 1;
    m_bConvertToVolt = false;
    m_bUseChExponent = false;
    m_bUseUnitGain = false;
    m_bUseUnitOffset = false;
    m_bWriteToFile = false;
    m_bIsRunning = false;
    m_sOutputFilePath = QString("mne_x_plugins/resources/tmsi");
}


//*************************************************************************************************************

bool TMSI::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    //Set the channel size of the RMTSA - this needs to be done here and NOT in the init() function because the user can change the number of channels during runtime
    m_pRMTSA_TMSI->data()->init(m_iNumberOfChannels);
    m_pRMTSA_TMSI->data()->setSamplingRate(m_iSamplingFreq);
    m_pRMTSA_TMSI->data()->setVisibility(true);

    //Buffer
    m_pRawMatrixBuffer_In = QSharedPointer<RawMatrixBuffer>(new RawMatrixBuffer(8, m_iNumberOfChannels, m_iSamplesPerBlock));

    m_pTMSIProducer->start(m_iNumberOfChannels,
                           m_iSamplingFreq,
                           m_iSamplesPerBlock,
                           m_bConvertToVolt,
                           m_bUseChExponent,
                           m_bUseUnitGain,
                           m_bUseUnitOffset,
                           m_bWriteToFile,
                           m_sOutputFilePath);

    if(m_pTMSIProducer->isRunning())
    {
        m_bIsRunning = true;
        QThread::start();
        return true;
    }
    else
    {
        qWarning() << "Plugin TMSI - ERROR - TMSIProducer thread could not be started - Either the device is turned off (check your OS device manager) or the driver DLL (RTINST.dll) is not installed in the system directory" << endl;
        return false;
    }
}


//*************************************************************************************************************

bool TMSI::stop()
{
    m_bIsRunning = false;

    //Stop producer thread
    m_pTMSIProducer->stop();

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
    TMSISetupWidget* widget = new TMSISetupWidget(this);//widget is later destroyed by CentralWidget - so it has to be created everytime new

    //init properties dialog
    widget->initSamplingProperties();

    return widget;
}


//*************************************************************************************************************

void TMSI::run()
{
    while(m_bIsRunning)
    {
        //pop matrix
        MatrixXf matValue = m_pRawMatrixBuffer_In->pop();
        //std::cout << "matValue " << matValue.block(0,0,m_iNumberOfChannels,m_iSamplesPerBlock) << std::endl;

        //emit values to real time multi sample array
        for(qint32 i = 0; i < matValue.cols(); ++i)
        {
            //Check if one sample (values for all channels at one "sample moment" in time) is equal to zero -> if so the application is reading faster from the buffer than the device can write new data into the buffer -> do not display these zero values
            if(!matValue.col(i).isZero())
                m_pRMTSA_TMSI->data()->setValue(matValue.col(i).cast<double>());
        }
    }


}
