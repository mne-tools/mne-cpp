#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include <map>
#include <set>

namespace EVENTSLIB {

class EVENTS_EXPORT EventManager
{
public:
    EventManager();

    void saveSelectedGroup();

    void addEvent(int iSample);

    void addGroup(const char* sGroupName);

private:

    EVENTSINTERNAL::EventGroup          m_iSelectedGroup;

    std::set<EVENTSINTERNAL::Event>             m_eventList;
    std::map<int, EVENTSINTERNAL::EventGroup>   m_eventGroupList;
};

}//namespace
#endif // EVENTS_H
