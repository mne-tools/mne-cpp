//=============================================================================================================
/**
 * @file     event.h
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 * @since    0.1.8
 * @date     February, 2021
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
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
 * @brief Public event descriptor holding sample index, group membership, and unique identifier
 *
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

    idNum  id;      /**< Unique key of this event object. Not the trigger code, see eventCode. */
    int  sample;    /**< First sample covered by the event. */
    int  duration;  /**< Length in samples. Zero for an instantaneous event. */
    int  eventCode; /**< FIFF trigger code identifying the kind of event. */
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
 * @brief Internal event representation with sample position, group link, and unique ID used by EventManager
 *
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
     * Returns the event code, which identifies what kind of event this is.
     *
     * This is the value FIFF calls the event id and stores in the third
     * column of an event matrix, i.e. the trigger code. It is what
     * distinguishes one stimulus condition from another, and is not related
     * to getId(), which is only a unique key for this event object.
     *
     * @return The event code. One by default.
     */
    int getEventCode() const;

    //=========================================================================================================
    /**
     * Set the event code.
     *
     * @param iEventCode The trigger code identifying the kind of event.
     */
    void setEventCode(int iEventCode);

    //=========================================================================================================
    /**
     * Returns the length of the event in samples.
     *
     * Zero means an instantaneous event, which is the default. A positive
     * value makes the event span [sample, sample + duration), which is how a
     * FIFF annotation with a non zero duration is represented.
     *
     * @return Event duration in samples.
     */
    int getDuration() const;

    //=========================================================================================================
    /**
     * Set the length of the event in samples.
     *
     * Negative values are clamped to zero, since an event cannot end before
     * it starts.
     *
     * @param iDuration Duration in samples.
     */
    void setDuration(int iDuration);

    //=========================================================================================================
    /**
     * Sample one past the last sample covered by this event.
     *
     * For an instantaneous event this equals getSample().
     *
     * @return End sample of the event.
     */
    int getEndSample() const;

    //=========================================================================================================
    /**
     * Whether this event covers a range of samples rather than a single one.
     *
     * @return True when the duration is greater than zero.
     */
    bool isRanged() const;

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
    int         m_iSample;                  /**< First sample covered by the event */
    int         m_iDuration;                /**< Length in samples. Zero for an instantaneous event. */
    int         m_iEventCode;               /**< FIFF trigger code identifying the kind of event. */
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

