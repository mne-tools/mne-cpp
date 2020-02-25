//=============================================================================================================
/**
 * @file     dummytoolbox.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @version  dev
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Christoph Dinh, Lorenz Esch, Viktor Klueber. All rights reserved.
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
 * @brief    Definition of the DummyToolbox class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dummytoolbox.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DUMMYTOOLBOXPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DummyToolbox::DummyToolbox()
: m_bIsRunning(false)
, m_pDummyInput(NULL)
, m_pDummyOutput(NULL)
, m_pDummyBuffer(CircularMatrixBuffer<double>::SPtr())
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionShowYourWidget = new QAction(QIcon(":/images/options.png"), tr("Your Toolbar Widget"),this);
    m_pActionShowYourWidget->setShortcut(tr("F12"));
    m_pActionShowYourWidget->setStatusTip(tr("Your Toolbar Widget"));
    connect(m_pActionShowYourWidget, &QAction::triggered,
            this, &DummyToolbox::showYourWidget);
    addPluginAction(m_pActionShowYourWidget);
}

//=============================================================================================================

DummyToolbox::~DummyToolbox()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<IPlugin> DummyToolbox::clone() const
{
    QSharedPointer<DummyToolbox> pDummyToolboxClone(new DummyToolbox);
    return pDummyToolboxClone;
}

//=============================================================================================================

void DummyToolbox::init()
{
    // Input
    m_pDummyInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "DummyIn", "Dummy input data");
    connect(m_pDummyInput.data(), &PluginInputConnector::notify, this, &DummyToolbox::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pDummyInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pDummyOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "DummyOut", "Dummy output data");
    m_outputConnectors.append(m_pDummyOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pDummyBuffer.isNull())
        m_pDummyBuffer = CircularMatrixBuffer<double>::SPtr();
}

//=============================================================================================================

void DummyToolbox::unload()
{
}

//=============================================================================================================

bool DummyToolbox::start()
{
//    //Check if the thread is already or still running. This can happen if the start button is pressed immediately after the stop button was pressed. In this case the stopping process is not finished yet but the start process is initiated.
//    if(this->isRunning())
//        QThread::wait();

    m_bIsRunning = true;

    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool DummyToolbox::stop()
{
    m_bIsRunning = false;

    m_pDummyBuffer->releaseFromPop();
    m_pDummyBuffer->releaseFromPush();

    m_pDummyBuffer->clear();

    return true;
}

//=============================================================================================================

IPlugin::PluginType DummyToolbox::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString DummyToolbox::getName() const
{
    return "Dummy Toolbox";
}

//=============================================================================================================

QWidget* DummyToolbox::setupWidget()
{
    DummySetupWidget* setupWidget = new DummySetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void DummyToolbox::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pDummyBuffer) {
            m_pDummyBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pDummyOutput in the constructor above
            m_pDummyOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pDummyOutput->data()->setMultiArraySize(1);
            m_pDummyOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pDummyBuffer->push(&t_mat);
        }
    }
}

//=============================================================================================================

void DummyToolbox::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pDummyBuffer->pop();

        //ToDo: Implement your algorithm here

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pDummyOutput in the constructor above
        m_pDummyOutput->data()->setValue(t_mat);
    }
}

//=============================================================================================================

void DummyToolbox::showYourWidget()
{
    m_pYourWidget = DummyYourWidget::SPtr(new DummyYourWidget());
    m_pYourWidget->show();
}
