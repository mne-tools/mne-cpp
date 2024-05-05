//=============================================================================================================
/**
 * @file     timefrequency.cpp
 * @author   Juan Garcia-Prieto <jgarciaprieto@nmr.mgh.harvard.edu>;
 * @since    0.1.0
 * @date     March, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Christoph Dinh, Gabriel B Motta, Lorenz Esch. All rights reserved.
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

#include "timefrequency/timefrequency.h"

#include <utils/ioutils.h>

#include <scMeas/realtimemultisamplearray.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QString>
#include <QLabel>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

namespace TIMEFREQUENCYPLUGIN {

TimeFrequency::TimeFrequency()
: m_pRTMSA_In(Q_NULLPTR),
  m_pRTMSA_Out(Q_NULLPTR)
{
    qDebug() << "[TimeFrequency::TimeFrequency] Creating Plugin Object.";
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{
    if(this->isRunning()) {
        TimeFrequency::stop();
    }
}

//=============================================================================================================

QSharedPointer<SCSHAREDLIB::AbstractPlugin> TimeFrequency::clone() const
{
    QSharedPointer<TimeFrequency> pTimeFrequencyClone(new TimeFrequency);
    return pTimeFrequencyClone;
}

//=============================================================================================================

void TimeFrequency::init()
{
    //  Input
    m_pRTMSA_In = SCSHAREDLIB::PluginInputData<
        SCMEASLIB::RealTimeMultiSampleArray>::
        create(this, "TimeFrequencyIn", "TimeFrequency in data");
    connect(m_pRTMSA_In.data(), &SCSHAREDLIB::PluginInputConnector::notify,
            this, &TimeFrequency::update, Qt::DirectConnection);
    m_inputConnectors.append(m_pRTMSA_In);

    //  Output
    m_pRTMSA_Out = SCSHAREDLIB::PluginOutputData<
    SCMEASLIB::RealTimeMultiSampleArray>::
        create(this, "TimeFrequencyOut", "TimeFrequencyPluginOutputdata");
    m_pRTMSA_Out->measurementData()->setName(this->getName());
    m_outputConnectors.append(m_pRTMSA_Out);
}

//=============================================================================================================

void TimeFrequency::unload()
{

}

//=============================================================================================================

bool TimeFrequency::start()
{
    //Start thread as soon as we have received the first data block. See update().
    return true;
}

//=============================================================================================================

bool TimeFrequency::stop()
{
    requestInterruption();
   
    wait(500);

    return true;
}

//=============================================================================================================

SCSHAREDLIB::AbstractPlugin::PluginType TimeFrequency::getType() const
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
    return new QLabel("Time Frequency Baby");
}

//=============================================================================================================
void TimeFrequency::update(QSharedPointer<SCMEASLIB::Measurement> pMeasurement)
{
    qDebug() << "new data!";
}

void TimeFrequency::run()
{

    // Init
    Eigen::MatrixXd matData;
    // matData.
    while (true)
    {
        msleep(500);
        if(isInterruptionRequested())
            break;
        qDebug() << "run!!";

        // m_pRTMSA_Out->measurementData()->setValue(matData);
    }
}

//=============================================================================================================

QString TimeFrequency::getBuildInfo()
{
    return QString(buildDateTime()) + QString(" - ")  + QString(buildHash()); 
}

}  // namespace TIMEFREQUENCYPLUGIN

