//=============================================================================================================
/**
 * @file     abstractplugin.cpp
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Juan Garcia-Prieto. All rights reserved.
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
 * @brief    Definition of the AbstractPlugin class.
 *
 */
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractplugin.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractPlugin::AbstractPlugin()
: m_bInitialized(false)
, m_bMenuAlreadyLoaded(false)
, m_bViewAlreadyLoaded(false)
, m_bControlAlreadyLoaded(false)
, m_iOrder(0)
{
}

//=============================================================================================================

AbstractPlugin::~AbstractPlugin()
{
}

//=============================================================================================================

void AbstractPlugin::setInitState(bool b)
{
    m_bInitialized = b;
}

//=============================================================================================================

bool AbstractPlugin::hasBeenInitialized() const
{
    return m_bInitialized;
}


//=============================================================================================================

void AbstractPlugin::cmdLineStartup(const QStringList& sArguments)
{
    Q_UNUSED(sArguments)
}

//=============================================================================================================

void AbstractPlugin::setGlobalData(QSharedPointer<AnalyzeData> globalData)
{
    m_pAnalyzeData = globalData;
}

//=============================================================================================================

int AbstractPlugin::getOrder() const
{
    return m_iOrder;
}

//=============================================================================================================

void AbstractPlugin::setOrder(int order)
{
    m_iOrder = order;
}

//=============================================================================================================

bool AbstractPlugin::viewAlreadyLoaded() const
{
    return m_bViewAlreadyLoaded;
}

//=============================================================================================================

void AbstractPlugin::setViewLoadingState(bool b)
{
    m_bViewAlreadyLoaded = b;
}

//=============================================================================================================

bool AbstractPlugin::controlAlreadyLoaded() const
{
    return m_bControlAlreadyLoaded;
}

//=============================================================================================================

void AbstractPlugin::setControlLoadingState(bool b)
{
    m_bControlAlreadyLoaded = b;
}

//=============================================================================================================

bool AbstractPlugin::menuAlreadyLoaded() const
{
    return m_bMenuAlreadyLoaded;
}

//=============================================================================================================

void AbstractPlugin::setMenuLoadingState(bool b)
{
    m_bMenuAlreadyLoaded = b;
}
