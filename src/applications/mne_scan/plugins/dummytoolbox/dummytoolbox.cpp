//=============================================================================================================
/**
 * @file     dummytoolbox.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Viktor Klueber <Viktor.Klueber@tu-ilmenau.de>
 * @since    0.1.0
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
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DUMMYTOOLBOXPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DummyToolbox::DummyToolbox()
: m_pCircularBuffer(QSharedPointer<CircularBuffer_Matrix_double>(new CircularBuffer_Matrix_double(40)))
{
}

//=============================================================================================================

DummyToolbox::~DummyToolbox()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> DummyToolbox::clone() const
{
    QSharedPointer<DummyToolbox> pClone(new DummyToolbox);
    return pClone;
}

//=============================================================================================================

QString DummyToolbox::getBuildInfo()
{
    return QString(DUMMYTOOLBOXPLUGIN::buildDateTime()) + QString(" - ") + QString(DUMMYTOOLBOXPLUGIN::buildHash());
}

//=============================================================================================================

void DummyToolbox::init()
{
    // Input
    m_pInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "DummyIn", "Dummy input data");
    connect(m_pInput.data(), &PluginInputConnector::notify,
            this, &DummyToolbox::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pInput);

    // Output - Uncomment this if you don't want to send processed data (in form of a matrix) to other plugins.
    // Also, this output stream will generate an online display in your plugin
    m_pOutput = PluginOutputData<RealTimeMultiSampleArray>::create(this, "DummyOut", "Dummy output data");
    m_pOutput->measurementData()->setName(this->getName());
    m_outputConnectors.append(m_pOutput);
}

//=============================================================================================================

void DummyToolbox::unload()
{
}

//=============================================================================================================

bool DummyToolbox::start()
{
    //Start thread
    QThread::start();

    return true;
}

//=============================================================================================================

bool DummyToolbox::stop()
{
    requestInterruption();
    wait(500);

    // Clear all data in the buffer connected to displays and other plugins
    m_pOutput->measurementData()->clear();
    m_pCircularBuffer->clear();

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType DummyToolbox::getType() const
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
    DummySetupWidget* setupWidget = new DummySetupWidget(this);
    return setupWidget;
}

//=============================================================================================================

void DummyToolbox::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();

            m_pOutput->measurementData()->initFromFiffInfo(m_pFiffInfo);
            m_pOutput->measurementData()->setMultiArraySize(1);
        }

        if(!m_bPluginControlWidgetsInit) {
            initPluginControlWidgets();
        }

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

void DummyToolbox::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // The plugin's control widget
        DummyYourWidget* pYourWidget = new DummyYourWidget(QString("MNESCAN/%1").arg(this->getName()));
        pYourWidget->setObjectName("group_tab_Settings_Your Widget");

        plControlWidgets.append(pYourWidget);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());

        m_bPluginControlWidgetsInit = true;
    }
}

//=============================================================================================================

void DummyToolbox::run()
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
                m_pOutput->measurementData()->setValue(matData);
            }
        }
    }
}
