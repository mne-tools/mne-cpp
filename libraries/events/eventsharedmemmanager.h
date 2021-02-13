#ifndef EVENTSHAREDMEMMANAGER_EVENTS_H
#define EVENTSHAREDMEMMANAGER_EVENTS_H

#include <string>
#include <memory>
#include <stack>
#include <QSharedMemory>

#include "event.h"
#include "eventgroup.h"

namespace EVENTSLIB {

enum SharedMemoryMode { READ, WRITE };
class EventManager;
}

namespace EVENTSINTERNAL {

enum EventUpdateType { AddEvent, DeleteEvent };
struct EventUpdate
{
    EventUpdateType type;
    int             sample;
    //timestamp
};

class EventSharedMemManager
{
public:

    EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);
    bool init(EVENTSLIB::SharedMemoryMode mode);
    bool stop();

    bool isInit() const;

private:
    EVENTSLIB::EventManager*        m_parent;
    QSharedMemory                   m_SharedMemory;
    bool                            m_bIsInit;
    std::string                     m_sGroupName;
    EventUpdate*                    m_localBuffer;

 //   int                             m_iTimeLastCheck;

 //create a buffer std::queue of EventUpdate objs.
};

} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H
