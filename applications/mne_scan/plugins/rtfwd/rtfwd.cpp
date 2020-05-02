//=============================================================================================================
/**
 * @file     rtfwd.cpp
 * @author   Ruben Dörfel <ruben.doerfel@tu-ilmenau.de>
 * @since    0.1.0
 * @date     May, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief    Definition of the RtFwd class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtfwd.h"
#include <fwd/computeFwd/compute_fwd.h>
#include <fwd/computeFwd/compute_fwd_settings.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTFWDPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace Eigen;
using namespace FWDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RtFwd::RtFwd()
: m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr::create(40))
{
}

//=============================================================================================================

RtFwd::~RtFwd()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> RtFwd::clone() const
{
    QSharedPointer<RtFwd> pClone(new RtFwd);
    return pClone;
}

//=============================================================================================================

void RtFwd::init()
{
    // Inits
    m_pFwdSettings = ComputeFwdSettings::SPtr(new ComputeFwdSettings);

    // Input
    m_pInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "rtFwdIn", "rtFwd input data");
    connect(m_pInput.data(), &PluginInputConnector::notify,
            this, &RtFwd::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "rtFwdOut", "rtFwd output data");
    m_pOutput->data()->setName(this->getName());
    m_outputConnectors.append(m_pOutput);
}

//=============================================================================================================

void RtFwd::unload()
{
}

//=============================================================================================================

bool RtFwd::start()
{
    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool RtFwd::stop()
{
    requestInterruption();
    wait(500);

    // Clear all data in the buffer connected to displays and other plugins
    m_pOutput->data()->clear();
    m_pCircularBuffer->clear();

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

IPlugin::PluginType RtFwd::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString RtFwd::getName() const
{
    return "Real-Time Forward Solution";
}

//=============================================================================================================

QWidget* RtFwd::setupWidget()
{
    RtFwdSetupWidget* setupWidget = new RtFwdSetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void RtFwd::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pOutput->data()->initFromFiffInfo(m_pFiffInfo);
            m_pOutput->data()->setMultiArraySize(1);
        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

        MatrixXd matData;

        for(unsigned char i = 0; i < pRTMSA->getMultiArraySize(); ++i) {
            // Please note that we do not need a copy here since this function will block until
            // the buffer accepts new data again. Hence, the data is not deleted in the actual
            // Mesaurement function after it emitted the notify signal.
            while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                //Do nothing until the circular buffer is ready to accept new data again
            }
        }
    }
}

//=============================================================================================================

void RtFwd::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // The plugin's control widget
//        RtFwdWidget* pYourWidget = new RtFwdWidget(QString("MNESCAN/%1/").arg(this->getName()));
//        pYourWidget->setObjectName("group_tab_Settings_Your Widget");

//        plControlWidgets.append(pYourWidget);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void RtFwd::run()
{
    MatrixXd matData;

    // Wait for Fiff Info
    while(!m_pFiffInfo) {
        msleep(10);
    }

    while(!isInterruptionRequested()) {
        // Get the current data
        if(m_pCircularBuffer->pop(matData)) {
            //ToDo: Implement your algorithm here

            //Send the data to the connected plugins and the online display
            //Unocmment this if you also uncommented the m_pOutput in the constructor above
            if(!isInterruptionRequested()) {
                m_pOutput->data()->setValue(matData);
            }
        }
    }
}
