//=============================================================================================================
/**
* @file     {{source_filename}}
* @author   {{author}} <{{author_email}}>
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     {{month}}, {{year}}
*
* @section  LICENSE
*
* Copyright (C) {{year}}, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the {{name}} class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "{{header_filename}}"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace {{namespace}};
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

{{name}}::{{name}}()
: m_bIsRunning(false)
, m_pBuffer(CircularMatrixBuffer<double>::SPtr())
, m_pInput(NULL)
, m_pOutput(NULL)
{
    //Add action which will be visible in the plugin's toolbar
    m_pActionShowWidget = new QAction(QIcon(":/images/options.png"), tr("{{name}} Toolbar Widget"),this);
    m_pActionShowWidget->setShortcut(tr("F12"));
    m_pActionShowWidget->setStatusTip(tr("{{name}} Toolbar Widget"));
    connect(m_pActionShowWidget, &QAction::triggered, this, &{{name}}::showWidget);
    addPluginAction(m_pActionShowWidget);
}


//*************************************************************************************************************

{{name}}::~{{name}}()
{
    if(this->isRunning())
        stop();
}


//*************************************************************************************************************

QSharedPointer<IPlugin> {{name}}::clone() const
{
    QSharedPointer<{{name}}> pClone(new {{name}});
    return pClone;
}


//*************************************************************************************************************

void {{name}}::init()
{
    // Input
    m_pInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "{{name}}In", "{{name}} input data");
    connect(m_pInput.data(), &PluginInputConnector::notify, this, &{{name}}::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "{{name}}Out", "{{name}} output data");
    m_outputConnectors.append(m_pOutput);

    //Delete Buffer - will be initailzed with first incoming data
    if(!m_pBuffer.isNull())
        m_pBuffer = CircularMatrixBuffer<double>::SPtr();
}


//*************************************************************************************************************

void {{name}}::unload()
{

}


//*************************************************************************************************************

bool {{name}}::start()
{
    // Check if the thread is already or still running. 
    // This can happen if the start button is pressed immediately after the stop button was pressed. 
    // In this case the stopping process is not finished yet but the start process is initiated.
    if(this->isRunning())
        QThread::wait();

    m_bIsRunning = true;
    QThread::start();
    return true;
}


//*************************************************************************************************************

bool {{name}}::stop()
{
    m_bIsRunning = false;
    m_pBuffer->releaseFromPop();
    m_pBuffer->releaseFromPush();
    m_pBuffer->clear();
    return true;
}


//*************************************************************************************************************

IPlugin::PluginType {{name}}::getType() const
{
    return _IAlgorithm;
}


//*************************************************************************************************************

QString {{name}}::getName() const
{
    return "Dummy Toolbox";
}


//*************************************************************************************************************

QWidget* {{name}}::setupWidget()
{
    {{setup_widget_name}}* setupWidget = new {{setup_widget_name}}(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}


//*************************************************************************************************************

void {{name}}::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>();

    if(pRTMSA) {
        //Check if buffer initialized
        if(!m_pBuffer) {
            m_pBuffer = CircularMatrixBuffer<double>::SPtr(new CircularMatrixBuffer<double>(64, pRTMSA->getNumChannels(), pRTMSA->getMultiSampleArray()[0].cols()));
        }

        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            //Init output - Unocmment this if you also uncommented the m_pOutput in the constructor above
            m_pOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pOutput->data()->setMultiArraySize(1);
            m_pOutput->data()->setVisibility(true);
        }

        MatrixXd t_mat;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            t_mat = pRTMSA->getMultiSampleArray()[i];
            m_pBuffer->push(&t_mat);
        }
    }
}



//*************************************************************************************************************

void {{name}}::run()
{
    // Wait for Fiff Info
    while(!m_pFiffInfo)
        msleep(10);

    while(m_bIsRunning)
    {
        //Dispatch the inputs
        MatrixXd t_mat = m_pBuffer->pop();

        //ToDo: Implement your algorithm here

        //Send the data to the connected plugins and the online display
        //Unocmment this if you also uncommented the m_pDummyOutput in the constructor above
        m_pOutput->data()->setValue(t_mat);
    }
}


//*************************************************************************************************************

void {{name}}::showWidget()
{
    m_pWidget = {{widget_name}}::SPtr(new {{widget_name}}());
    m_pWidget->show();
}
