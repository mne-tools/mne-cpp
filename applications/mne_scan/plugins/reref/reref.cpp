//=============================================================================================================
/**
* @file     reref.cpp
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
* @brief    Contains the implementation of the reref class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "reref.h"
#include "FormFiles/rerefsetupwidget.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace REREFPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace IOBUFFER;
using namespace std;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

ReRef::ReRef()
: m_bIsRunning(false)
, m_pRTMSA(NewRealTimeMultiSampleArray::SPtr(new NewRealTimeMultiSampleArray()))
, m_bDisp(true)
{
}


//*************************************************************************************************************

ReRef::~ReRef()
{
    if(this->isRunning()) {
        stop();
    }
}


//*************************************************************************************************************

QSharedPointer<IPlugin> ReRef::clone() const
{
    QSharedPointer<ReRef> pReRefClone(new ReRef);
    return pReRefClone;
}


//*************************************************************************************************************

void ReRef::init()
{
    // Input
    m_pReRefInput = PluginInputData<NewRealTimeMultiSampleArray>::create(this, "ReRefIn", "ReRef input data");
    connect(m_pReRefInput.data(), &PluginInputConnector::notify,
            this, &ReRef::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pReRefInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    m_pReRefOutput = PluginOutputData<NewRealTimeMultiSampleArray>::create(this, "ReRefOut", "ReRef output data");
    m_outputConnectors.append(m_pReRefOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pReRefBuffer.isNull())
        m_pReRefBuffer = CircularMatrixBuffer<double>::SPtr();


}


//*************************************************************************************************************

void ReRef::unload()
{

}


//*************************************************************************************************************

bool ReRef::start()
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

bool ReRef::stop()
{
    m_bIsRunning = false;

    m_pReRefBuffer->releaseFromPop();
    m_pReRefBuffer->clear();

    return true;
}


//*************************************************************************************************************

IPlugin::PluginType ReRef::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString ReRef::getName() const
{
    return "ReRef";
}


//*************************************************************************************************************

QWidget* ReRef::setupWidget()
{
    //widget is later distroyed by CentralWidget - so it has to be created everytime new
    ReRefSetupWidget* setupWidget = new ReRefSetupWidget(this);
    return setupWidget;
}


//*************************************************************************************************************

void ReRef::update(SCMEASLIB::NewMeasurement::SPtr pMeasurement)
{
//    QSharedPointer<NewRealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    m_pRTMSA = pMeasurement.dynamicCast<NewRealTimeMultiSampleArray>();

    if(m_pRTMSA) {
        //Check if buffer initialized
        if(!m_pReRefBuffer) {
            m_pReRefBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, m_pRTMSA->getNumChannels(), m_pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = m_pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pReRefOutput in the constructor above
            m_pReRefOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pReRefOutput->data()->setMultiArraySize(1);
            m_pReRefOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < m_pRTMSA->getMultiArraySize(); ++i) {
            t_mat = m_pRTMSA->getMultiSampleArray()[i];
            m_pReRefBuffer->push(&t_mat);
        }
    }
}


//*************************************************************************************************************

void ReRef::run()
{
    //
    // Wait for Fiff Info
    //
    while(!m_pFiffInfo) {
        msleep(10);// Wait for fiff Info
    }

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pReRefBuffer->pop();

        m_mutex.lock();

        MatrixXd matCAR = EEGRef::applyCAR(t_mat, m_pFiffInfo);

        if(m_bDisp){

            IOUtils::write_eigen_matrix(t_mat, "matIN.txt");
            IOUtils::write_eigen_matrix(matCAR, "matOUT.txt");

            m_bDisp = false;
        }

        m_mutex.unlock();

        //Send the data to the connected plugins and the online display
        m_pReRefOutput->data()->setValue(matCAR);
    }
}
