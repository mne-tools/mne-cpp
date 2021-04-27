//=============================================================================================================
/**
 * @file     event.h
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>;
 * @since    0.1.8
 * @date     February, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief     Event declaration.
 *
 */

#ifndef EVENT_EVENTSINTERNAL_H
#define EVENT_EVENTSINTERNAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events_global.h"
#include "eventgroup.h"

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// NAMESPACE EVENTSLIB
//=============================================================================================================

namespace EVENTSLIB {

//=============================================================================================================
// EVENTSINTERNAL FORWARD DECLARATIONS
//=============================================================================================================

namespace EVENTSINTERNAL {
    class EventINT;
}

//=============================================================================================================
/**
 * This is a public Class to organize events and make it easy to manipulate
 * for the end-user of the library.
 */
struct EVENTS_EXPORT Event
{
    //=========================================================================================================
    /**
     * EventINT constructor.
     */
    Event();

    //=========================================================================================================
    /**
     * Event class constructor
     *
     * @param[in] id Id of the event to be created.
     * @param[in] sample Sample number of the event to be created.
     * @param[in] groupId GroupId of the EventGroupINT to which the event will belong to.
     */
    Event(const idNum id,const  int sample, const idNum groupId);

    //=========================================================================================================
    /**
     * Event constructor based on an internal event object of class EventINT.
     *
     * @param[in] e An event object.
     */
    Event(const EVENTSINTERNAL::EventINT& e);

    idNum  id;      /**< Event id. */
    int  sample;    /**< Sample number of the event. */
    idNum  groupId; /**< GroupId of this event. */
};

namespace EVENTSINTERNAL {
// The fact that we go with int for sample is a fundamental limitation of this
// whole architecture. With a Fs = 1kHz, we could have a maximum of aprox. 25 days.
// Yes not a big limitation... if we keep using 1kHz...
// If we were to go for long longs for sample... with that same 1kHz, we could go
// recording, single file... for about 300 million years. That's that...
// at some point we can substitute int and idNum with std::int64_t. That will take that limitation away.
// I don't see how I should ever think of this again.
/**
 * The EventINT class
 *
 * The events are objects of this class internally in the library.
 */
class EventINT
{
public:
    //=========================================================================================================
    /**
     * EventINT constructor.
     *
     * @param[in] Id of the new event.
     */
    EventINT(idNum id);

    //=========================================================================================================
    /**
     * EventINT constructor.
     *
     * @param[in] id Id of the new event.
     * @param[in] iSample Sample of the new created event.
     * @param[in] groupId GroupId to which the created event will belong.
     */
    EventINT(idNum id, int iSample, idNum groupId);

    //=========================================================================================================
    /**
     * Copy constructor for the EventINT class.
     *
     * @param rhs Rhs EventINT object.
     */
    EventINT(const EventINT& rhs);

    //=========================================================================================================
    /**
     * Move constructor
     *
     * @param other Object to be moved.
     */
    EventINT(EventINT&& other);

    //=========================================================================================================
    /**
     * Create an event at sample iSample
     *
     * @param[in] iSample
     */
    static inline EventINT fromSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event sample.
     *
     * @return Event sample.
     */
    int getSample() const;

    //=========================================================================================================
    /**
     * setSample Set the value of the event sample.
     *
     * @param iSample sample.
     */
    void setSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event group
     *
     * @return Event group.
     */
    idNum getGroupId() const;

    //=========================================================================================================
    /**
     * Set the value of the group of this event.
     *
     * @param iGroup Group id.
     */
    void setGroupId(idNum iGroup);

    //=========================================================================================================
    /**
     * Retrieve this event's id.
     *
     * @return Id Event Id.
     */
    idNum getId() const;

    //=========================================================================================================
    /**
     * Retrieve this event's description.
     *
     * @return Event Description.
     */
    std::string getDescription() const;

    //=========================================================================================================
    /**
     * Set this event's description.
     *
     * @param[in] description The new description text.
     */
    void setDescription(const std::string& description);

    //=========================================================================================================
    /**
     * Set this event's description from a rvalue string.
     *
     * @param[in] description The new description.
     */
    void setDescription(std::string&& description);

    //=========================================================================================================
    /**
     * Overriden < operator. This operator helps the organization of events in standard library containers.
     *
     * @param[in] rhs EventINT to compare to.
     *
     * @return Bool value with the result of the comparison.
     */
    bool operator<(const EventINT& rhs) const;

    //=========================================================================================================
    /**
     * Overriden == operator. This operator helps the organization of events in standard library containers.
     *
     * @param[in] rhs EventINT to compare to.
     *
     * @return Bool value with the result of the comparison.
     */
    bool operator==(const EventINT& rhs) const;

    //=========================================================================================================
    /**
     * Overriden = operator in case of need to copy assing an event.
     *
     * @param rhs EventINT to copy assign to.
     *
     * @return a new EventINT created.
     */
    EventINT operator=(const EventINT& rhs);

private:
    idNum       m_iId;                      /**< Placeholder for sample Id */
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */
    idNum       m_iGroup;                   /**< Group this event belongs to */
    std::string m_sDescription;             /**< Short string describing info */
};

//=========================================================================================================
/**
 * Create an EventINT event from a specific sample.
 *
 * @param[in] sample Sample of the new event.
 *
 * @return new event created.
 */
inline EventINT EventINT::fromSample(int sample)
{
    return EventINT(0, sample, 0);
}

}//namespace EVENTSINTERNAL
}//namespace EVENTSLIB

//=========================================================================================================

/**
 * Template specialization for the EventINT class. Helpful when dealing with std library containers.
 */
namespace std {
template<>
struct hash<EVENTSLIB::EVENTSINTERNAL::EventINT>
{
    size_t operator()(const EVENTSLIB::EVENTSINTERNAL::EventINT& rhs) const
    {
        return hash<int>()(rhs.getId());
    }
};

}

#endif // EVENT_H


