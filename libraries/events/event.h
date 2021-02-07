#ifndef EVENT_EVENTSINTERNAL_H
#define EVENT_EVENTSINTERNAL_H

#include "events_global.h"

using uint = unsigned int;

namespace EVENTSINTERNAL {
class Event;
}

namespace EVENTSLIB {

struct EVENTS_EXPORT Event
{
    Event();
    Event(const uint i,const  int s, const uint gid);
    Event(const EVENTSINTERNAL::Event&);

    uint  id;
    uint  groupId;
    int  sample;
};

}

namespace EVENTSINTERNAL {

class EventGroup;

class Event
{
public:
    //=========================================================================================================
    /**
     * Create an event at sample iSample
     *
     * @param iSample
     * @param group
     */
    Event(int iSample, const EventGroup& group);

    //=========================================================================================================
    /**
     * Returns event sample
     *
     * @return event sample
     */
    int getSample() const;

    //=========================================================================================================
    void setSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event group
     *
     * @return event group
     */
    uint getGroup() const;

    //=========================================================================================================
    void setGroup(uint iGroup);

    uint getId() const;

    bool operator<(const Event& rhs) const;

private:
    uint        m_iId;                      /**< Sample Id */
    uint        m_iGroup;                   /**< Group the event belongs to */
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */

    static int eventIdCounter;
};

}
#endif // EVENT_H


