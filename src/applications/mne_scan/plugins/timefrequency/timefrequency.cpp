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
#include <scShared/plugins/abstractplugin.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

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
: m_pTimeFrequencyInput(Q_NULLPTR)
// : m_bCompActivated(false)
// , m_bSpharaActive(false)
// , m_bProjActivated(false)
// , m_bFilterActivated(false)
// , m_iMaxFilterLength(1)
// , m_iMaxFilterTapSize(-1)
// , m_sCurrentSystem("VectorView")
// , m_pCircularBuffer(QSharedPointer<UTILSLIB::CircularBuffer_Matrix_double>::create(40))
{
    qDebug() << "[TimeFrequency::TimeFrequency] Creating Plugin Object.";
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{
    if(this->isRunning()) {
        stop();
    }
}

//=============================================================================================================

QSharedPointer<AbstractPlugin> TimeFrequency::clone() const
{
    QSharedPointer<TimeFrequency> pTimeFrequencyClone(new TimeFrequency);
    return pTimeFrequencyClone;
}

//=============================================================================================================

void TimeFrequency::init()
{
    // Input
    m_pTimeFrequencyInput = PluginInputData<RealTimeMultiSampleArray>::
        create(this, "TimeFrequencyIn", "TimeFrequency input data");
    m_inputConnectors.append(m_pTimeFrequencyInput);
    m_pTimeFrequencyInput->measurementData()->setName(this->getName());//Provide name to auto store widget settings
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
    QString* setupWidget("Time Frequency Baby");
    return setupWidget;
}

//=============================================================================================================

void TimeFrequency::run()
{
    // Read and create SPHARA operator for the first time
    // initSphara();
    // createSpharaOperator();

    // Init
    Eigen::MatrixXd matData;
    // matData.
    // QScopedPointer<RTPROCESSINGLIB::FilterOverlapAdd> pRtFilter(new RTPROCESSINGLIB::FilterOverlapAdd());

    while (true)
    {
        // Get the current data
        msleep(500);
        // Send the data to the connected plugins and the display
        if(!isInterruptionRequested()) {
            m_pTimeFrequencyInput->measurementData()->setValue(matData);
        }
    }
}

//=============================================================================================================

QString TimeFrequency::getBuildInfo()
{
    return QString(buildDateTime()) + QString(" - ")  + QString(buildHash()); 
}

}  // namespace TIMEFREQUENCYPLUGIN

