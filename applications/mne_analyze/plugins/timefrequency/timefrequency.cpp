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

#include <anShared/Management/communicator.h>
#include <anShared/Management/analyzedata.h>
#include <anShared/Model/fiffrawviewmodel.h>
#include <anShared/Model/annotationmodel.h>
#include <anShared/Model/averagingdatamodel.h>

#include <disp/viewers/progressview.h>
#include <disp/viewers/timefrequencyview.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace TIMEFREQUENCYPLUGIN;
using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

TimeFrequency::TimeFrequency()
: m_sSettingsPath("MNEANALYZE/TimeFrequency")
{
    loadSettings();
}

//=============================================================================================================

TimeFrequency::~TimeFrequency()
{

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
    m_pCommu = new Communicator(this);
}

//=============================================================================================================

void TimeFrequency::unload()
{

}

//=============================================================================================================

QString TimeFrequency::getName() const
{
    return "Time-Frequency";
}

//=============================================================================================================

QMenu *TimeFrequency::getMenu()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QDockWidget *TimeFrequency::getControl()
{
    return Q_NULLPTR;
}

//=============================================================================================================

QWidget *TimeFrequency::getView()
{
    m_pTimeFreqView = new DISPLIB::TimeFrequencyView();
    return m_pTimeFreqView;
}

//=============================================================================================================

void TimeFrequency::handleEvent(QSharedPointer<Event> e)
{

}

//=============================================================================================================

QVector<EVENT_TYPE> TimeFrequency::getEventSubscriptions(void) const
{
    QVector<EVENT_TYPE> temp = {SELECTED_MODEL_CHANGED};

    return temp;
}

//=============================================================================================================

void TimeFrequency::saveSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
}

//=============================================================================================================

void TimeFrequency::loadSettings()
{
    QSettings settings("MNECPP");
    settings.beginGroup(m_sSettingsPath);
}
