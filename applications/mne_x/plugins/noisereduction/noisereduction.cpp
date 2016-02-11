//=============================================================================================================
/**
* @file     noisereduction.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the NoiseReduction class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "noisereduction.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace NoiseReductionPlugin;
using namespace MNEX;
using namespace XMEASLIB;
using namespace IOBuffer;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NoiseReduction::NoiseReduction()
: m_bIsRunning(false)
, m_pNoiseReductionInput(NULL)
, m_pNoiseReductionOutput(NULL)
, m_pNoiseReductionBuffer(CircularMatrixBuffer<double>::SPtr())
, m_iNBaseFcts(500)
, m_bSpharaActive(false)
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionShowOptionsWidget = new QAction(QIcon(":/images/options.png"), tr("Noise reduction options"),this);
    m_pActionShowOptionsWidget->setShortcut(tr("F12"));
    m_pActionShowOptionsWidget->setStatusTip(tr("Noise reduction options"));
    connect(m_pActionShowOptionsWidget, &QAction::triggered,
            this, &NoiseReduction::showOptionsWidget);
    addPluginAction(m_pActionShowOptionsWidget);
}


//*************************************************************************************************************

NoiseReduction::~NoiseReduction()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> NoiseReduction::clone() const
{
    QSharedPointer<NoiseReduction> pNoiseReductionClone(new NoiseReduction);
    return pNoiseReductionClone;
}


//*************************************************************************************************************

void NoiseReduction::init()
{
    // Input
    m_pNoiseReductionInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "NoiseReductionIn", "NoiseReduction input data");
    connect(m_pNoiseReductionInput.data(), &PluginInputConnector::notify, this, &NoiseReduction::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pNoiseReductionInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pNoiseReductionOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "NoiseReductionOut", "NoiseReduction output data");
    m_outputConnectors.append(m_pNoiseReductionOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pNoiseReductionBuffer.isNull())
        m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void NoiseReduction::unload()
{

}


//*************************************************************************************************************

bool NoiseReduction::start()
{
    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}


//*************************************************************************************************************

bool NoiseReduction::stop()
{
    m_bIsRunning = false;

    m_pNoiseReductionBuffer->releaseFromPop();
    m_pNoiseReductionBuffer->releaseFromPush();

    m_pNoiseReductionBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType NoiseReduction::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString NoiseReduction::getName() const
{
    return "NoiseReduction Toolbox";
}


//*************************************************************************************************************

QWidget* NoiseReduction::setupWidget()
{
    NoiseReductionSetupWidget* setupWidget = new NoiseReductionSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void NoiseReduction::update(XMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pNoiseReductionBuffer) {
            m_pNoiseReductionBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pNoiseReductionOutput in the constructor above
            m_pNoiseReductionOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pNoiseReductionOutput->data()->setMultiArraySize(1);
            m_pNoiseReductionOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pNoiseReductionBuffer->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void NoiseReduction::setSpharaMode(bool state)
{
    m_mutex.lock();
    m_bSpharaActive = state;
    qDebug()<<"NoiseReduction::setSpharaMode:"<<state;
    m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::setSpharaNBaseFcts(int nBaseFcts)
{
    //m_mutex.lock();
    m_iNBaseFcts = nBaseFcts;
    qDebug()<<"NoiseReduction::setSpharaNBaseFcts:"<<nBaseFcts;
    //m_mutex.unlock();
}


//*************************************************************************************************************

void NoiseReduction::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pNoiseReductionBuffer->pop();

        m_mutex.lock();
        //To all the noise reduction steps here
        //SPHARA calculations
        if(m_bSpharaActive) {

        }

        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pNoiseReductionOutput in the constructor above
        m_pNoiseReductionOutput->data()->setValue(t_mat);
    }
}


//*************************************************************************************************************

void NoiseReduction::showOptionsWidget()
{
    m_pOptionsWidget = NoiseReductionOptionsWidget::SPtr(new NoiseReductionOptionsWidget(this));
    m_pOptionsWidget->show();
}
