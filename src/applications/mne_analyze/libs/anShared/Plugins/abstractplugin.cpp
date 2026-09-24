//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     abstractplugin.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.9
 * @date     April, 2021
 * @brief    Definition of the AbstractPlugin class.
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
