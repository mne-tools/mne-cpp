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
#include <scMeas/realtimemultisamplearray.h>

#include <iostream>

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
: m_pCircularEvokedBuffer(CircularBuffer<FIFFLIB::FiffEvoked>::SPtr::create(40))
, m_iDataQueueBlockSize(25)
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
    if(QSharedPointer<RealTimeEvokedSet> pRTES = pMeasurement.dynamicCast<RealTimeEvokedSet>()) {
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTES->info();
        }

        FIFFLIB::FiffEvokedSet::SPtr pFiffEvokedSet = pRTES->getValue();

        if(this->isRunning()) {
            for(int i = 0; i < pFiffEvokedSet->evoked.size(); ++i) {
                while(!m_pCircularEvokedBuffer->push(pFiffEvokedSet->evoked.at(i))) {
                    //Do nothing until the circular buffer is ready to accept new data again
                }
            }
        }
    }
    if(QSharedPointer<RealTimeMultiSampleArray> pRTMSA = pMeasurement.dynamicCast<RealTimeMultiSampleArray>()) {
         //Fiff information
        if(!m_pFiffInfo) {
            m_pFiffInfo = pRTMSA->info();
        }

//        if (!m_pRTTF){
//        }

        if (m_pFiffInfo){
            QMutexLocker locker(&m_qMutex);
            for(unsigned char i = 0; i < pRTMSA->getMultiSampleArray().size(); ++i) {
                // Please note that we do not need a copy here since this function will block until
                // the buffer accepts new data again. Hence, the data is not deleted in the actual
                // Measurement function after it emitted the notify signal.
                m_DataQueue.push_back(pRTMSA->getMultiSampleArray()[i]);
//                while(!m_pCircularTimeSeriesBuffer->push(pRTMSA->getMultiSampleArray()[i])) {
//                    //Do nothing until the circular buffer is ready to accept new data again
//                }
            }
        }
    }
}

//=============================================================================================================

void TimeFrequency::init()
{
    // Input
    m_pTimeFrequencyEvokedInput = PluginInputData<RealTimeEvokedSet>::create(this, "TfEvokedIn", "Time frequency input data");
    connect(m_pTimeFrequencyEvokedInput.data(), &PluginInputConnector::notify,
            this, &TimeFrequency::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pTimeFrequencyEvokedInput);

    m_pTimeFrequencyTimeSeriesInput = PluginInputData<RealTimeMultiSampleArray>::create(this, "TfTimeSeiresIn", "Time frequency input data");
    connect(m_pTimeFrequencyTimeSeriesInput.data(), &PluginInputConnector::notify,
            this, &TimeFrequency::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pTimeFrequencyTimeSeriesInput);

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
    FIFFLIB::FiffEvoked evoked;
    QStringList lResponsibleTriggerTypes;

    while(!isInterruptionRequested()){
        if(m_DataQueue.size() > m_iDataQueueBlockSize){
            QMutexLocker locker(&m_qMutex);

            computeTimeFrequency();

            while(m_DataQueue.size() > m_iDataQueueBlockSize){
                m_DataQueue.pop_front();
            }
        }
//        if(m_pCircularEvokedBuffer->pop(evoked)) {
//            m_qMutex.lock();
//            lResponsibleTriggerTypes = m_lResponsibleTriggerTypes;
//            m_qMutex.unlock();

//            m_pTimeFrequencyOutput->measurementData()->setValue(evokedSet,
//                                                 m_pFiffInfo,
//                                                 lResponsibleTriggerTypes);
//        }
    }
}

//=============================================================================================================

void TimeFrequency::computeTimeFrequency()
{
    int iCols = 0;

    std::cout<< "First matrix r:" << m_DataQueue.front().rows() << " | c: " << m_DataQueue.front().cols() << std::endl;

    for (auto mat : m_DataQueue){
        iCols += mat.cols();
    }

    std::cout << "COLS: " << iCols << std::endl;

    Eigen::MatrixXd dataMat(m_DataQueue.front().rows(), iCols);

    for (auto mat : m_DataQueue){
        dataMat << mat;
    }

    std::cout << "New Matrix - r: " << dataMat.rows() << " | c: " << dataMat.cols() << std::endl;
}
