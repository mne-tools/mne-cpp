#ifndef EVENTSHAREDMEMMANAGER_EVENTS_H
#define EVENTSHAREDMEMMANAGER_EVENTS_H

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <QSharedMemory>

#include "event.h"
#include "eventgroup.h"

namespace EVENTSLIB {

enum SharedMemoryMode { READ, WRITE, BYDIRECTIONAL };
class EventManager;
}

namespace EVENTSINTERNAL {

class EventUpdate
{
public:
    enum type{NewEvent, DeleteEvent};

    EventUpdate();
    EventUpdate(int sample, int creator,type t);

    long long getCreationTime() const;

    int getSample() const;

    int getCreatorId() const;

    EventUpdate::type getType() const;

    void setType(type t);

protected:
    int                 m_EventSample;
    int                 m_CreatorId;
    long long           m_CreationTime;
    EventUpdate::type   m_TypeOfUpdate;

};

class EventSharedMemManager
{
public:
    EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);
    ~EventSharedMemManager();
    void init(EVENTSLIB::SharedMemoryMode mode);

    bool isInit() const;
    void stop();
    void addEvent(int sample);
    void deleteEvent(int sample);

    static long long getTimeNow();

    void processEvent(const EventUpdate& ne);

private:
    inline static int generateId();
    std::string EventTypeToText(EventUpdate::type t);
    void processNewEvent(const EventUpdate& n);
    void processDeleteEvent(const EventUpdate& n);
    void printLocalBuffer();
    void storeUpdateInSharedMemory(const EventUpdate& nu);
    void copyLocalBufferToSharedMemory();
    void copySharedMemoryToLocalBuffer();
    void processLocalBuffer();
    void bufferWatcher();
    void createGroupIfNeeded();

    static int                      m_iLastUpdateIndex;
    EVENTSLIB::EventManager*        m_pEventManager;
    QSharedMemory                   m_SharedMemory;
    bool                            m_bIsInit;
    std::string                     m_sGroupName;
    bool                            m_bGroupCreated;
    idNum                           m_GroupId;
    float                           m_fTimerCheckBuffer;
    std::thread                     m_BufferWatcherThread;
    long long                       m_lastCheckTime;
    EventUpdate*                    m_LocalBuffer;
    EventUpdate*                    m_SharedBuffer;
    int                             m_Id;
    enum EVENTSLIB::SharedMemoryMode m_Mode;
};

inline int EventSharedMemManager::generateId()
{
    auto t = static_cast<int>(EventSharedMemManager::getTimeNow());
    return abs(t);
}

} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H
