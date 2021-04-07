//=============================================================================================================
/**
 * @file     communicator.h
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
 * @brief    Communicator class declaration.
 *
 */

#ifndef ANSHAREDLIB_COMMUNICATOR_H
#define ANSHAREDLIB_COMMUNICATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "event.h"
#include "../Utils/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class AbstractPlugin;

//=========================================================================================================
/**
 * DECLARE CLASS Communicator
 *
 * @brief Communicator class for Event communication
 */
class ANSHAREDSHARED_EXPORT Communicator : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<Communicator> SPtr;            /**< Shared pointer type for Communicator. */
    typedef QSharedPointer<const Communicator> ConstSPtr; /**< Const shared pointer type for Communicator. */

    typedef long CommunicatorID;                        /**< Typedef for CommunicatorID. */

    //=========================================================================================================
    /**
     * Constructs a Communicator object that emits a signal (receivedEvent) when one of the passed list of events
     * happens. A further QtConnect IS necessary (See implementation of second constructor for more details.
     *
     * @param[in] subs           The list of relevant events.
     */
    Communicator(const QVector<EVENT_TYPE>& subs = QVector<EVENT_TYPE>());

    //=========================================================================================================
    /**
     * Constructs a Communicator object that is connected to the Plugins' handleEvent method.
     *
     * @param[in] plugin      The Plugins to connect to.
     */
    Communicator(AbstractPlugin* plugin);

    //=========================================================================================================
    /**
     * Destructs the communicator and disconnects it from the EventManager
     */
    ~Communicator();

    //=========================================================================================================
    /**
     * Sends an Event of type etype into the event system
     *
     * @param[in] etype          Type of the event to be published.
     * @param[in] data           Potential data to be attached to the event.
     */
    void publishEvent(EVENT_TYPE etype, const QVariant& data = QVariant()) const;

    //=========================================================================================================
    /**
     * Overwrites the Communicators subscriptions. Attention: old subscriptions will be deleted!
     * See addSubscriptions.
     *
     * @param[in] subs           The new list of Event types to be notified about.
     */
    void updateSubscriptions(const QVector<EVENT_TYPE>& subs);

    //=========================================================================================================
    /**
     * Adds the provided list of Event types to the preexisting list.
     *
     * @param[in] newsubs        List of new subscriptions.
     */
    void addSubscriptions(const QVector<EVENT_TYPE>& newsubs);

    //=========================================================================================================
    /**
     * Convenience overload, see addSubscriptions
     *
     * @param[in] newsub         New subscription to be added.
     */
    void addSubscriptions(EVENT_TYPE newsub);

    //=========================================================================================================
    /**
     * Manually disconnects a Communicator from the Event system.
     */
    void manualDisconnect(void);

    //=========================================================================================================
    /**
     * Getter for list of subscriptions
     *
     * @return List of subscriptions.
     */
    inline const QVector<EVENT_TYPE> getSubscriptions(void) const;

    //=========================================================================================================
    /**
     * Getter for internal ID
     *
     * @return Internal ID.
     */
    inline CommunicatorID getID(void) const;

signals:
    /**
     * Called by EventManager whenever an event needs to be handled. This must be connected to some other
     * function for actual usage.
     *
     * @param[in] e              The event that was received.
     */
    void receivedEvent(const QSharedPointer<Event> e);

private:
    static CommunicatorID m_IDCounter;                  /**< ID-Counter for Communicator instances. */
    inline static CommunicatorID nextID();              /**< Simply increments the counter and returns it. */

    CommunicatorID m_ID;                                /**< Communicator ID. */
    QVector<EVENT_TYPE> m_EventSubscriptions;           /**< All event types that the Communicator receives*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline Communicator::CommunicatorID Communicator::nextID()
{
    return ++m_IDCounter;
}

//=============================================================================================================

inline const QVector<EVENT_TYPE> Communicator::getSubscriptions(void) const
{
    return m_EventSubscriptions;
}

//=============================================================================================================

inline Communicator::CommunicatorID Communicator::getID(void) const
{
    return m_ID;
}

} // namespace

#endif // ANSHAREDLIB_COMMUNICATOR_H
