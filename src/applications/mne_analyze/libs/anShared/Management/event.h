//=============================================================================================================
/**
 * @file     event.h
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
 * @brief    Event class declaration.
 *
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
