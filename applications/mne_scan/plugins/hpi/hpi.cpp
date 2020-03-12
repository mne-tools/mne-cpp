//=============================================================================================================
/**
 * @file     hpi.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @version  dev
 * @date     February, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch. All rights reserved.
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
 * @brief    Definition of the Hpi class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "hpi.h"

#include "FormFiles/hpisetupwidget.h"

#include <disp/viewers/hpisettingsview.h>
#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace HPIPLUGIN;
using namespace SCMEASLIB;
using namespace IOBUFFER;
using namespace DISPLIB;
using namespace FIFFLIB;
using namespace SCSHAREDLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Hpi::Hpi()
: m_pCircularBuffer(CircularBuffer_Matrix_double::SPtr(new CircularBuffer_Matrix_double(40)))
{
}

//=============================================================================================================

Hpi::~Hpi()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<IPlugin> Hpi::clone() const
{
    QSharedPointer<Hpi> pHpiClone(new Hpi);
    return pHpiClone;
}

//=============================================================================================================

void Hpi::init()
{
    // Input
    m_pHpiInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "HpiIn", "Hpi input data");
    connect(m_pHpiInput.data(), &PluginInputConnector::notify,
            this, &Hpi::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pHpiInput);
}

//=============================================================================================================

void Hpi::unload()
{
}

//=============================================================================================================

bool Hpi::start()
{
    QThread::start();

    return true;
}

//=============================================================================================================

bool Hpi::stop()
{
    requestInterruption();
    wait();

    return true;
}

//=============================================================================================================

IPlugin::PluginType Hpi::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString Hpi::getName() const
{
    return "HPI Fitting";
}

//=============================================================================================================

QWidget* Hpi::setupWidget()
{
    HpiSetupWidget* setupWidget = new HpiSetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void Hpi::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
        //Check if the fiff info was inititalized
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();
            initPluginControlWidgets();
        }

        // Check if data is present
        if(pRTMSA->getMultiSampleArray().size() > 0) {
            for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                // Please note that we do not need a copy here since this function will block until
                // the buffer accepts new data again. Hence, the data is not deleted in the actual
                // Mesaurement function after it emitted the notify signal.
                while(!m_pCircularBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }
        }
    }
}

//=============================================================================================================

void Hpi::initPluginControlWidgets()
{
    if(m_pFiffInfo) {
        QList<QWidget*> plControlWidgets;

        // Projects Settings
        HpiSettingsView* pHpiSettingsView = new HpiSettingsView();
        pHpiSettingsView->setObjectName("widget_");

        plControlWidgets.append(pHpiSettingsView);

        emit pluginControlWidgetsChanged(plControlWidgets, this->getName());
    }
}

//=============================================================================================================

void Hpi::run()
{
    MatrixXd matData;
    qint32 size = 0;

    while(!isInterruptionRequested()) {
        if(m_pCircularBuffer) {
            //pop matrix
            if(m_pCircularBuffer->pop(matData)) {
                // Perform HPI fit
            }
        }
    }
}
