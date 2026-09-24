//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     communicator.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2018
 * @brief    Implementation of the Communicator class
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "communicator.h"
#include "eventmanager.h"
#include "../Plugins/abstractplugin.h"

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

Communicator::Communicator(const QVector<EVENT_TYPE> &subs)
: m_ID(nextID())
, m_EventSubscriptions(subs)
{
    EventManager::addCommunicator(this);
}

//=============================================================================================================

Communicator::Communicator(AbstractPlugin* plugin)
: Communicator(plugin->getEventSubscriptions())
{
    QObject::connect(this, &Communicator::receivedEvent,
                     plugin, &AbstractPlugin::handleEvent);
}

//=============================================================================================================

Communicator::~Communicator()
{
    EventManager::removeCommunicator(this);
}

//=============================================================================================================

void Communicator::publishEvent(EVENT_TYPE etype, const QVariant &data) const
{
    // simply wrap in smart pointer, fill in the sender pointer, and pass on to EventManager
    EventManager::issueEvent(QSharedPointer<Event>::create(etype, this, data));
}

//=============================================================================================================

void Communicator::updateSubscriptions(const QVector<EVENT_TYPE> &subs)
{
    // update routing table of event manager
    EventManager::updateSubscriptions(this, subs);
    // update own subscription list: This HAS to be done AFTER the EventManager::updateSubscriptions,
    // since the latter uses the communicators old list in order to keep execution time low
    m_EventSubscriptions.clear();
    m_EventSubscriptions.append(subs);
}

//=============================================================================================================

void Communicator::addSubscriptions(const QVector<EVENT_TYPE> &newsubs)
{
    m_EventSubscriptions.append(newsubs);
    // add new subscriptions to routing table of event manager
    EventManager::addSubscriptions(this, newsubs);
}

//=============================================================================================================

void Communicator::addSubscriptions(EVENT_TYPE newsub)
{
    // convenience function, simply wrap in vector
    addSubscriptions(QVector<EVENT_TYPE>{newsub});
}

//=============================================================================================================

void Communicator::manualDisconnect(void)
{
    // simply delegate to EventManager
    EventManager::removeCommunicator(this);
}

//=============================================================================================================

//=============================================================================================================
// DEFINE STATIC MEMBERS
//=============================================================================================================

Communicator::CommunicatorID Communicator::m_IDCounter;
