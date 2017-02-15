//=============================================================================================================
/**
* @file     reference.cpp
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
* @brief    Contains the implementation of the Reference class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "reference.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REFERENCEPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Reference::Reference()
: m_bIsRunning(false)
, m_pRefInput(NULL)
, m_pRefOutput(NULL)
, m_pRefBuffer(CircularMatrixBuffer<double>::SPtr())
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionRefToolbarWidget = new QAction(QIcon(":/images/options.png"), tr("Reference Toolbar"),this);
    m_pActionRefToolbarWidget->setShortcut(tr("F12"));
    m_pActionRefToolbarWidget->setStatusTip(tr("Reference Toolbar"));
    connect(m_pActionRefToolbarWidget, &QAction::triggered,
            this, &Reference::showRefToolbarWidget);
    addPluginAction(m_pActionRefToolbarWidget);
}


//*************************************************************************************************************

Reference::~Reference()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> Reference::clone() const
{
    QSharedPointer<Reference> pRefClone(new Reference);
    return pRefClone;
}


//*************************************************************************************************************

void Reference::init()
{
    // Input
    m_pRefInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "ReferenceIn", "Reference input data");
    connect(m_pRefInput.data(), &PluginInputConnector::notify, this, &Reference::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRefInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pRefOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "ReferenceOut", "Reference output data");
    m_outputConnectors.append(m_pRefOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pRefBuffer.isNull())
        m_pRefBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void Reference::unload()
{

}


//*************************************************************************************************************

bool Reference::start()
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

bool Reference::stop()
{
    m_bIsRunning = false;

    m_pRefBuffer->releaseFromPop();
    m_pRefBuffer->releaseFromPush();

    m_pRefBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType Reference::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString Reference::getName() const
{
    return "EEG Reference";
}


//*************************************************************************************************************

QWidget* Reference::setupWidget()
{
    ReferenceSetupWidget* setupWidget = new ReferenceSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void Reference::update(SCMEASLIB::NewMeasurement::SPtr pMeasurement)
{
    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pRefBuffer) {
            m_pRefBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pRefOutput in the constructor above
            m_pRefOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pRefOutput->data()->setMultiArraySize(1);
            m_pRefOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pRefBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************

void Reference::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo)
        msleep(10);// Wait for fiff Info

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pRefBuffer->pop();

        // apply common average reference
        MatrixXd matCAR = EEGRef::applyCAR(t_mat, m_pFiffInfo);

//        // write in- and output matrix to a file
//        if(m_bDisp){

//            IOUtils::write_eigen_matrix(t_mat, "matIN.txt");
//            IOUtils::write_eigen_matrix(matCAR, "matOUT.txt");

//            m_bDisp = false;
//        }

        //Send the data to the connected plugins and the online display
        m_pRefOutput->data()->setValue(matCAR);
    }
}


//*************************************************************************************************************

void Reference::showRefToolbarWidget()
{
    m_pRefToolbarWidget = ReferenceToolbarWidget::SPtr(new ReferenceToolbarWidget(this));
    m_pRefToolbarWidget->show();
}
