//=============================================================================================================
/**
 * @file     timefrequency.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the TimeFrequency class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "timefrequency.h"
#include "FormFiles/timefrequencysetupwidget.h"

#include <disp/viewers/timefrequencysettingsview.h>

#include <scMeas/realtimeevokedset.h>
#include <scMeas/realtimetimefrequency.h>

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

using namespace TIMEFREQUENCYPLUGIN;
using namespace SCSHAREDLIB;
using namespace SCMEASLIB;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequency::TimeFrequency()
: m_pCircularBuffer(CircularBuffer<FIFFLIB::FiffEvokedSet>::SPtr::create(40))
{
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{
    if(this->isRunning())
        stop();
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TimeFrequency::clone() const
{
    QSharedPointer<TimeFrequency> pTimeFrequencyClone(new TimeFrequency);
    return pTimeFrequencyClone;
}

//=============================================================================================================

void TimeFrequency::unload()
{
}

//=============================================================================================================

bool TimeFrequency::start()
{
    QThread::start();

    return true;
}

//=============================================================================================================

bool TimeFrequency::stop()
{    
    requestInterruption();
    wait(500);

    m_bPluginControlWidgetsInit = false;

    return true;
}

//=============================================================================================================

AbstractPlugin::PluginType TimeFrequency::getType() const
{
    return _IAlgorithm;
}

//=============================================================================================================

QString TimeFrequency::getName() const
{
    return "TimeFrequency";
}

//=============================================================================================================

QWidget* TimeFrequency::setupWidget()
{
    TimeFrequencySetupWidget* setupWidget = new TimeFrequencySetupWidget(this);//widget is later distroyed by CentralWidget - so it has to be created everytime new
    return setupWidget;
}

//=============================================================================================================

void TimeFrequency::update(SCMEASLIB::Measurement::SPtr pMeasurement)
{

}

//=============================================================================================================

void TimeFrequency::init()
{
    // Input
    m_pTimeFrequencyInput = PluginInputData<RealTimeEvokedSet>::create(this, "TfIn", "Time frequency input data");

    connect(m_pTimeFrequencyInput.data(), &PluginInputConnector::notify,
            this, &TimeFrequency::update, Qt::DirectConnection);

    //Output
    m_pTimeFrequencyOutput = PluginOutputData<RealTimeTimeFrequency>::create(this, "TfOut", "Time frequency output data");
    m_pTimeFrequencyOutput->measurementData()->setName(getName());
    m_outputConnectors.append(m_pTimeFrequencyOutput);
}

//=============================================================================================================

void TimeFrequency::initPluginControlWidgets()
{
    if(m_pFiffInfo) {

    }
}

//=============================================================================================================

void TimeFrequency::run()
{
    FIFFLIB::FiffEvokedSet evokedSet;
    QStringList lResponsibleTriggerTypes;

    while(!isInterruptionRequested()){
//        if(m_pCircularBuffer->pop(evokedSet)) {
//            m_qMutex.lock();
//            lResponsibleTriggerTypes = m_lResponsibleTriggerTypes;
//            m_qMutex.unlock();

//            m_pTimeFrequencyOutput->measurementData()->setValue(evokedSet,
//                                                 m_pFiffInfo,
//                                                 lResponsibleTriggerTypes);
//        }
    }
}
