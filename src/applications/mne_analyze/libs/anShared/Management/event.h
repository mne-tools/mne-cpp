//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     event.h
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Simon Heinke <Simon.Heinke@tu-ilmenau.de>
 * @since    0.1.0
 * @date     April, 2018
 * @brief    Event class declaration.
 */

#ifndef ANSHAREDLIB_EVENT_H
#define ANSHAREDLIB_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../anshared_global.h"
#include "../Utils/types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>
#include <QPointer>
#include <QVariant>

//=============================================================================================================
// DEFINE NAMESPACE ANSHAREDLIB
//=============================================================================================================

namespace ANSHAREDLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class Communicator;

//=========================================================================================================
/**
 * DECLARE CLASS Event
 *
 * @brief Event class for inter-Plugin communication. Basically holds type information, sender and data.
 */
class ANSHAREDSHARED_EXPORT Event : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<Event> SPtr;            /**< Shared pointer type for Event. */
    typedef QSharedPointer<const Event> ConstSPtr; /**< Const shared pointer type for Event. */

    //=========================================================================================================
    /**
     * Constructs an Event object.
     *
     * @param[in] type       The type of the event.
     * @param[in] sender     The sender of the message.
     * @param[in] data       Data that may be attached to the event.
     */
    Event(const EVENT_TYPE type, const Communicator* sender, const QVariant& data);

    //=========================================================================================================
    /**
     * @brief Destructor
     */
    ~Event() = default;

    //=========================================================================================================
    /**
     * Getter for Event type.
     *
     * @return Type of the Event.
     */
    inline EVENT_TYPE getType() const;

    //=========================================================================================================
    /**
     * Getter for Event Sender
     *
     * @return Sender of the Event.
     */
    inline const Communicator *getSender() const;

    //=========================================================================================================
    /**
     * Getter for Event data.
     *
     * @return Data of the Event.
     */
    inline QVariant getData() const;

private:
    EVENT_TYPE m_eventType;             /**< Type of the respective Event instance. */
    const Communicator* m_sender;       /**< Sender of the Event. */
    const QVariant m_data;              /**< Attached Data (can be empty). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline EVENT_TYPE Event::getType() const
{
    return m_eventType;
}

//=============================================================================================================

inline const Communicator* Event::getSender() const
{
    return m_sender;
}

//=============================================================================================================

QVariant Event::getData() const
{
    return m_data;
}

} // NAMESPACE

#endif // ANSHAREDLIB_EVENT_H
