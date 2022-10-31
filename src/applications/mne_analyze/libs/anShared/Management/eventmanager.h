//=============================================================================================================
/**
 * @file     eventmanager.h
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
 * @brief    EventManager class declaration.
 *
 */

#ifndef ANSHAREDLIB_EVENT_MANAGER_H
#define ANSHAREDLIB_EVENT_MANAGER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "event.h"
#include "../Utils/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QSemaphore>
#include <QPointer>
#include <QMultiMap>
#include <QThread>
#include <QQueue>
#include <QMutex>

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Communicator;

//=============================================================================================================
/**
 * DECLARE CLASS EventManager
 *
 * EvenManager for Event communication. Holds a routing table that keeps track of Event subscriptions.
 * Event-ownership is still a bit problematic: The communicator simply wraps the passed data in a shared pointer
 * and passes it on to the EventManager. There it gets stored in a queue until the next processing circle. It
 * then is dequeued and distributed to all subscribed communicators.
 */
class ANSHAREDSHARED_EXPORT EventManager : public QThread
{
    Q_OBJECT

public:
    typedef QSharedPointer<EventManager> SPtr;            /**< Shared pointer type for EventManager. */
    typedef QSharedPointer<const EventManager> ConstSPtr; /**< Const shared pointer type for EventManager. */

    //=========================================================================================================
    /**
     * Delete copy constructor (singleton)
     */
    EventManager(const EventManager& other) = delete;

    //=========================================================================================================
    /**
     * Adds a Communicator, respectively its subscriptions to the routing table
     *
     * @param[in] commu          The Communicator to add.
     */
    static void addCommunicator(Communicator* commu);

    //=========================================================================================================
    /**
     * Communicate an event to all entities that have registered for the respective event type.
     * The Event will get buffered in a queue and processed during the next processing cycle.
     *
     * @param[in] e              The event to publish
     */
    static void issueEvent(QSharedPointer<Event> e);

    //=========================================================================================================
    /**
     * Expands a Communicator's subscriptions by the specified list
     *
     * @param[in] commu          The Communicator to add the events for
     * @param[in] newsubs        List of new (additional) subscriptions
     */
    static void addSubscriptions(Communicator* commu, QVector<EVENT_TYPE> newsubs);

    //=========================================================================================================
    /**
     * Replaces a Communicators subscriptions with the specified list.
     *
     * @param[in] commu          The respective Communicator
     * @param[in] subs           New list of subscriptions
     */
    static void updateSubscriptions(Communicator* commu, const QVector<EVENT_TYPE> &subs);

    //=========================================================================================================
    /**
     * Removes (and thus disconnects) a Communicator and all its subscriptions from the routing table
     *
     * @param[in] commu          The communicator to remove.
     */
    static void removeCommunicator(Communicator* commu);

    //=========================================================================================================
    /**
     * Starts the EventManagers thread that processes buffered events. The EventManager will try to maintain the
     * specified frequency of dealing with buffered events (frequency in Hz)
     *
     * @param frequency          The frequency in Hz to start working through all buffered events.
     * @return                   Whether starting was successfull
     */
    static bool startEventHandling(float frequency = 25.0f);

    //=========================================================================================================
    /**
     * Stops the EventThread. Depending on the specified event-processing frequency, this might take some time
     * (up to one waiting period, to be precise. Example: EventManager running on 20 Hz -> up to 50 ms shutdown).
     *
     * @return                   Whether stopping was successfull
     */
    static bool stopEventHandling();

    //=========================================================================================================
    /**
     * Indicates whether or not the EventManager currently holds any unprocessed events.
     *
     * @return Whether or not the EventManager has unprocessed events.
     */
    static bool hasBufferedEvents();

    //=========================================================================================================
    /**
     * This is called when the user presses the "close" button
     */
    static void shutdown();

private:
    //=========================================================================================================
    /**
     * Internal function to be called by addCommunicator. It adds a Communicator, respectively its subscriptions
     * to the routing table
     *
     * @param[in] commu          The Communicator to add
     */
    void addCommunicatorInt(Communicator* commu);

    //=========================================================================================================
    /**
     * Internal function to be called by issueEvent static function. Communicate an event to all entities that
     * have registered for the respective event type. The Event will get buffered in a queue and processed
     * during the next processing cycle.
     *
     * @param[in] e              The event to publish.
     */
    void issueEventInt(QSharedPointer<Event> e);

    //=========================================================================================================
    /**
     * Internal function to be called by addSubscriptions. Expands a Communicator's subscriptions by the
     * specified list
     *
     * @param[in] commu          The Communicator to add the events for.
     * @param[in] newsubs        List of new (additional) subscriptions.
     */
    void addSubscriptionsInt(Communicator* commu, QVector<EVENT_TYPE> newsubs);

    //=========================================================================================================
    /**
     * Internal function to be called by updateSubscription. Replaces a Communicators subscriptions with the
     * specified list.
     *
     * @param[in] commu          The respective Communicator.
     * @param[in] subs           New list of subscriptions.
     */
    void updateSubscriptionsInt(Communicator* commu, const QVector<EVENT_TYPE> &subs);

    //=========================================================================================================
    /**
     * Internal function to be called by removeCommunicator. Removes (and thus disconnects) a Communicator
     * and all its subscriptions from the routing table.
     *
     * @param[in] commu          The communicator to remove.
     */
    void removeCommunicatorInt(Communicator * commu);

    //=========================================================================================================
    /**
     * Internal function to be called by startEventHandling. Starts the EventManagers thread that processes
     * buffered events. The EventManager will try to maintain the specified frequency of dealing with
     * buffered events (frequency in Hz).
     *
     * @param[in] frequency          The frequency in Hz to start working through all buffered events.
     * @return                   Whether starting was successfull.
     */
    bool startEventHandlingInt(float frequency = 25.0f);

    //=========================================================================================================
    /**
     * Internal fcn to be called by stopEventHandling. Stops the EventThread. Depending on the specified
     * event-processing frequency, this might take some time (up to one waiting period, to be precise.
     * Example: EventManager running on 20 Hz -> up to 50 ms shutdown).
     *
     * @return                   Whether stopping was successfull.
     */
    bool stopEventHandlingInt();

    //=========================================================================================================
    /**
     * Internal fcn to be called by hasBufferedEvents. Indicates whether or not the EventManager currently
     * holds any unprocessed events.
     *
     * @return Whether or not the EventManager has unprocessed events.
     */
    bool hasBufferedEventsInt();

    //=========================================================================================================
    /**
     * Internal fcn to be called by shutdown(). This is called when the user presses the "close" button
     */
    void shutdownInt();

    //=========================================================================================================
    /**
     * Static method for singleton implementation (returns reference to local static object)
     *
     * @return A reference to the EventManager singleton
     */
    static EventManager& getEventManager();

    //=========================================================================================================
    /**
     * Private constructor (singleton)
     */
    EventManager();

    //=========================================================================================================
    /**
     * Overrides QThread::run. This executes the main loop for handling events. Never try to call this manually.
     */
    void run() override;

    QMultiMap<EVENT_TYPE, Communicator*>    m_routingTable;         /**< Map that holds routing information. */
    QQueue<QSharedPointer<Event> >          m_eventQ;               /**< Queue that buffers all published events. */

    QMutex                                  m_eventQMutex;          /**< Guarding mutex for the event queue. */
    QMutex                                  m_routingTableMutex;    /**< Guarding mutex for the routing table. */

    volatile long                           m_sleepTime;            /**< Controlling execution frequency of main loop. */
    volatile bool                           m_running;              /**< Flag for remembering whether the EventManager is currently started or stopped. */

    QSemaphore                              m_eventSemaphore;       /**< Keeps track of events and blocks. */
};

} // namespace

#endif // ANSHAREDLIB_EVENT_MANAGER_H
