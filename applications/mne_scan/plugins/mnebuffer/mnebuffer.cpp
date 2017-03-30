//=============================================================================================================
/**
* @file     mnebuffer.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Eric Larson <larson.eric.d@gmail.com>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     February, 2013
*
* @section  LICENSE
*
* Copyright (C) 2013, Christoph Dinh, Eric Larson and Matti Hamalainen. All rights reserved.
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
* @brief    Contains the implementation of the MneBuffer class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mnebuffer.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBUFFERPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MneBuffer::MneBuffer()
: m_bIsRunning(false)
, m_pMneBufferInput(NULL)
, m_pMneBufferOutput(NULL)
, m_pDataBuffer(CircularMatrixBuffer<double>::SPtr())
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionShowMneBufferWidget = new QAction(QIcon(":/images/options.png"), tr("MNE Buffer Widget"),this);
    m_pActionShowMneBufferWidget->setShortcut(tr("F12"));
    m_pActionShowMneBufferWidget->setStatusTip(tr("MNE BUFFER Widget"));
    connect(m_pActionShowMneBufferWidget, &QAction::triggered,
            this, &MneBuffer::showMneBufferWidget);
    addPluginAction(m_pActionShowMneBufferWidget);
}


//*************************************************************************************************************

MneBuffer::~MneBuffer()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> MneBuffer::clone() const
{
    QSharedPointer<MneBuffer> pMneBufferClone(new MneBuffer);
    return pMneBufferClone;
}


//*************************************************************************************************************

void MneBuffer::init()
{
    // Input
    m_pMneBufferInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "MneBufferIn", "MNE Buffer input data");
    connect(m_pMneBufferInput.data(), &PluginInputConnector::notify, this, &MneBuffer::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pMneBufferInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pMneBufferOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "MneBufferOut", "MNE Buffer output data");
    m_outputConnectors.append(m_pMneBufferOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pDataBuffer.isNull())
        m_pDataBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void MneBuffer::unload()
{

}


//*************************************************************************************************************

bool MneBuffer::start()
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

bool MneBuffer::stop()
{
    m_bIsRunning = false;

    m_pDataBuffer->releaseFromPop();
    m_pDataBuffer->releaseFromPush();

    m_pDataBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType MneBuffer::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString MneBuffer::getName() const
{
    return "MNE Buffer";
}


//*************************************************************************************************************

QWidget* MneBuffer::setupWidget()
{
    MneBufferSetupWidget* setupWidget = new MneBufferSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void MneBuffer::update(SCMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pDataBuffer) {
            m_pDataBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pDummyOutput in the constructor above
            m_pMneBufferOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pMneBufferOutput->data()->setMultiArraySize(1);
            m_pMneBufferOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pDataBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************

void MneBuffer::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pDataBuffer->pop();

        //ToDo: Implement your algorithm here

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pDummyOutput in the constructor above
        m_pMneBufferOutput->data()->setValue(t_mat);
    }
}


//*************************************************************************************************************

void MneBuffer::showMneBufferWidget()
{
    m_pMneBufferWidget = MneBufferWidget::SPtr(new MneBufferWidget());
    m_pMneBufferWidget->show();
}
