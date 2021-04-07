//=============================================================================================================
/**
 * @file     communicator.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Lorenz Esch, Lars Debor, Simon Heinke. All rights reserved.
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
 * @brief    Implementation of the Communicator class
 *
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
