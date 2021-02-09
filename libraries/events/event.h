#ifndef EVENT_EVENTSINTERNAL_H
#define EVENT_EVENTSINTERNAL_H

#include "events_global.h"
#include "eventgroup.h"
#include <string>

namespace EVENTSINTERNAL {
class Event;
}

namespace EVENTSLIB {

struct EVENTS_EXPORT Event
{
    Event();
    Event(const idNum idRHS,const  int sampleRHS, const idNum groupIdRHS);
    Event(const EVENTSINTERNAL::Event& e);

    idNum  id;
    idNum  groupId;
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
    explicit Event(idNum id, int iSample, idNum groupId);

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
    idNum getGroup() const;

    //=========================================================================================================
    void setGroup(idNum iGroup);

    idNum getId() const;

    bool operator<(const Event& rhs) const;

private:
    idNum       m_iId;                      /**< Placeholder for sample Id */
    idNum       m_iGroup;                   /**< Group the event belongs to */
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */
    std::string m_description;
    int         m_aux;
};

}
#endif // EVENT_H


