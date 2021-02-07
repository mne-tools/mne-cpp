#ifndef EVENTSHAREDMEMMANAGER_EVENTS_H
#define EVENTSHAREDMEMMANAGER_EVENTS_H

#include <string>

namespace EVENTSINTERNAL {

struct EventUpdate {
   //event
   //action create or delete event
   //epoch of change
};


class EventSharedMemManager
{
public:
    EventSharedMemManager();
    bool isInit() const;


private:
    bool            m_bSharedMemoryInitState;
    std::string     m_sGroupName;
    int             m_iTimeLastCheck;

 //create a buffer std::queue of EventUpdate objs.
};



} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H
