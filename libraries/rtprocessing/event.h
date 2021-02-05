//=============================================================================================================
/**
 * @file     event.h
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.8
 * @date     January, 2021
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
 * @brief     Event and EventList declarations.
 *
 */

#ifndef EVENT_RTPROCESSING_H
#define EVENT_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"
#include <set>
#include <vector>


//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QSharedMemory>


//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

class EventManager
{
    //make my boss happy.
};

}
class EventGroup
{
public:
    EventGroup(const char* name);

    void setColor(const char* color);
    std::string getName() const;

private:

    void setRandomColor();
    void setRandomId();

    std::string     m_sName;
    char            m_Color[4];
    int             m_Id;
};


class EventGroupList
{

public:
  std::vector<EventGroup> list;
};

// **Event Class** So far, only instanteous events. In the future suport for segment-events
// will need to be added.
/**
 * Class contaning infomation about an event
 */
class RTPROCESINGSHARED_EXPORT Event
{
public:
    //=========================================================================================================
    /**
     * Create an event at sample iSample
     *
     * @param [in] iSample  sample coorespondiong to this event
     */
    Event(int iSample);
    Event(int iSample, const EventGroup& group);
    Event(int iSample, const EventGroup& group, int iType);
//    Event(int iSampleStart, int iSampleEnd);
//    Event(int iSampleStart, int iSampleEnd, const EventGroup& group);
//    Event(int iSampleStart, int iSampleEnd, const EventGroup& group, int iType);

    //=========================================================================================================
    /**
     * Create and event with the same parameters as parameter event
     *
     * @param [in] event    event to be copied
     */
    Event(const Event &event);

    //=========================================================================================================
    /**
     * Returns event sample
     *
     * @return event sample
     */
    int getSample() const;

    //=========================================================================================================
    /**
     * Returns event type
     *
     * @return event type
     */
    int getType() const;

    //=========================================================================================================
    /**
     * Returns event group
     *
     * @return event group
     */
    int getGroup() const;

    bool operator<(const Event& rhs) const
    {
       return getSample() < rhs.getSample();
    }
private:
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */
    int         m_iSampleStart;             /**< Sample coorespodning to the start of the event */
    int         m_iSampleEnd;               /**< Sample coorespodning to the end of the event */
    int         m_iType;                /**< Type of the event */
    int         m_iGroup;               /**< Group the event belongs to */
    int         m_iId;
};

/**
 * Class containing a list of events
 */
class RTPROCESINGSHARED_EXPORT EventList
{
public:
    //=========================================================================================================
    /**
     * EventList Constructor
     */
    EventList();

    //=========================================================================================================
    /**
     * clears events in this instance of the event handler
     */
    void clear();

    //=========================================================================================================
    /**
     * Adds event to event handler and static event list
     *
     * @param event
     */
    void addEvent(const Event& event);

    //=========================================================================================================
    /**
     * Get number of events in static event list (total events added)
     *
     * @return number of events in static event list
     */
    int getNumberOfEvents() const;

    //=========================================================================================================
    /**
     * Returns the event at index iIndex
     *
     * @param [in] iIndex   index of the event to gets
     *
     * @return  event at iIndex
     */
    Event getEvent(int iIndex) const;

private:
    std::set<Event>     m_lEvents;          /**< List of events */
    QSharedMemory       m_SharedMemoryHandler;
};
}//namespace

#endif // EVENT_RTPROCESSING_H
