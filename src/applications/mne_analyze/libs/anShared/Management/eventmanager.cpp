//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     eventmanager.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2018
 * @brief    Implementation of the EventManager class
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventmanager.h"
#include "communicator.h"
#include <chrono>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QMutexLocker>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace ANSHAREDLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventManager::EventManager()
: m_routingTable()
, m_eventQ()
, m_eventQMutex()
, m_routingTableMutex()
, m_sleepTime(40l)
, m_running(false)
{

}

//=============================================================================================================

void EventManager::addCommunicator(Communicator *commu)
{
    getEventManager().addCommunicatorInt(commu);
}

//=============================================================================================================

void EventManager::addCommunicatorInt(Communicator* commu)
{
    QMutexLocker temp(&m_routingTableMutex);
    const QVector<EVENT_TYPE>& subscriptions = commu->getSubscriptions();
    for(const EVENT_TYPE& etype : subscriptions)
    {
        m_routingTable.insert(etype, commu);
    }
}

//=============================================================================================================

void EventManager::issueEvent(QSharedPointer<Event> e)
{
    getEventManager().issueEventInt(e);
}

//=============================================================================================================

void EventManager::issueEventInt(QSharedPointer<Event> e)
{
    QMutexLocker temp(&m_eventQMutex);
    m_eventQ.enqueue(e);
    m_eventSemaphore.release();
}

//=============================================================================================================

void EventManager::addSubscriptions(Communicator *commu,
                                    QVector<EVENT_TYPE> newsubs)
{
    getEventManager().addSubscriptionsInt(commu, newsubs);
}
//=============================================================================================================

void EventManager::addSubscriptionsInt(Communicator* commu,
                                       QVector<EVENT_TYPE> newsubs)
{
    QMutexLocker temp(&m_routingTableMutex);
    for(const EVENT_TYPE& etype : newsubs)
    {
        m_routingTable.insert(etype, commu);
    }
}

//=============================================================================================================

void EventManager::updateSubscriptions(Communicator* commu,
                                       const QVector<EVENT_TYPE> &subs)
{
    getEventManager().updateSubscriptionsInt(commu, subs);
}

//=============================================================================================================

void EventManager::updateSubscriptionsInt(Communicator* commu,
                                       const QVector<EVENT_TYPE> &subs)
{
    // remove all old subscriptions from EventManager routing table
    removeCommunicator(commu);
    // add new key-value-pairs into map
    addSubscriptions(commu, subs);
}

//=============================================================================================================

void EventManager::removeCommunicator(Communicator* commu)
{
    getEventManager().removeCommunicatorInt(commu);
}

//=============================================================================================================

void EventManager::removeCommunicatorInt(Communicator* commu)
{
    QMutexLocker temp(&m_routingTableMutex);
    for(const EVENT_TYPE& etype : commu->getSubscriptions())
    {
        int removed = m_routingTable.remove(etype, commu);
        // consistency check:
        if (removed != 1)
        {
            qDebug() << "[EventManager::removeCommunicator] WARNING ! Found " << removed << " entries instead of 1 for event type ";
            qDebug() << etype << " and communicator ID " << commu->getID();
        }
    }
}

//=============================================================================================================

bool EventManager::startEventHandling(float frequency)
{
    return getEventManager().startEventHandlingInt(frequency);
}

//=============================================================================================================

bool EventManager::startEventHandlingInt(float frequency)
{
    if (m_running)
    {
        qDebug() << "[EventManager::startEventHandling] WARNING ! somebody tried to call startEventHandling when already running...";
        return false;
    }
    else {
        m_sleepTime = (long) (1000.0f / frequency);
        m_running = true;
        // start qthread
        start();
        return true;
    }

}

//=============================================================================================================

bool EventManager::stopEventHandling()
{
    return getEventManager().stopEventHandlingInt();
}

//=============================================================================================================

bool EventManager::stopEventHandlingInt()
{
    if (m_running)
    {
        m_running = false;
        m_eventSemaphore.release();
        requestInterruption();
        wait();
        return true;
    }
    else {
        qDebug() << "[EventManager] WARNING ! Somebody tried to call stopEventHandling when already stopped...";
        return false;
    }
}

//=============================================================================================================

bool EventManager::hasBufferedEvents()
{
    return getEventManager().hasBufferedEventsInt();
}

//=============================================================================================================

bool EventManager::hasBufferedEventsInt()
{
    QMutexLocker temp(&m_eventQMutex);
    return (m_eventQ.isEmpty() == false);
}

//=============================================================================================================

EventManager& EventManager::getEventManager()
{
    // static singleton
    static EventManager em;
    return em;
}

//=============================================================================================================

void EventManager::run()
{
    // main loop
    while (true)
    {
        m_eventSemaphore.acquire();
        auto before = std::chrono::high_resolution_clock::now();
        // go through all buffered events:
        while (hasBufferedEvents() == true)
        {
            // safely remove first queue element
            QMutexLocker eventQLock(&m_eventQMutex);
            const QSharedPointer<Event> e = m_eventQ.dequeue();
            eventQLock.unlock();
            // safely extract list of subscribers
            QMutexLocker routingTableLock(&m_routingTableMutex);
            const QList<Communicator*> subscribers = m_routingTable.values(e->getType());
            for(Communicator* commu : subscribers)
            {
                // avoid self-messaging
                if (commu->getID() != e->getSender()->getID())
                {
                    // notify communicator about event
                    emit commu->receivedEvent(e);
                }
            }
            routingTableLock.unlock();
        }
        auto after = std::chrono::high_resolution_clock::now();
        long elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();
        if (m_sleepTime - elapsed > 0)
        {
            // still some time left in current cycle, sleep
            QThread::currentThread()->msleep(m_sleepTime - elapsed);
        }
        else
        {
            // issue warning
            qDebug() << "[EventManager::run] WARNING ! Running behind on event handling...";
        }
        // check for shutdown requests
        if (isInterruptionRequested())
        {
            return;
        }
    }
}

//=============================================================================================================

void EventManager::shutdown()
{
    getEventManager().shutdownInt();
}

//=============================================================================================================

void EventManager::shutdownInt()
{
    stopEventHandlingInt();
}
