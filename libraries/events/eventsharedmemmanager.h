#ifndef EVENTSHAREDMEMMANAGER_EVENTS_H
#define EVENTSHAREDMEMMANAGER_EVENTS_H

#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <QSharedMemory>

#include "event.h"
#include "eventgroup.h"

namespace EVENTSLIB {

enum SharedMemoryMode { READ, WRITE, READWRITE };
class EventManager;
}

namespace EVENTSINTERNAL {

class EventUpdate
{
public:
    enum type{ NULL_EVENT, NEW_EVENT, DELETE_EVENT};
    const char* typeString[3] = {"Null Event", "New Event", "Delete Event"};

    EventUpdate();
    EventUpdate(int sample, int creator,type t);

    long long getCreationTime() const;

    int getSample() const;

    int getCreatorId() const;

    EventUpdate::type getType() const;

    void setType(type t);

    std::string eventTypeToText();

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
    void attachToSharedSegment(QSharedMemory::AccessMode mode);
    bool createSharedSegment(int bufferSize, QSharedMemory::AccessMode mode);
    void launchSharedMemoryWatcherThread();
    void attachToOrCreateSharedSegment(QSharedMemory::AccessMode mode);
    void stopSharedMemoryWatcherThread();

    void detachFromSharedMemory();
    inline static int generateId();
    void processNewEvent(const EventUpdate& n);
    void processDeleteEvent(const EventUpdate& n);
    void printLocalBuffer();
    void copyNewUpdateToSharedMemory(EventUpdate& newUpdate);
    void initializeSharedMemory();
    void copySharedMemoryToLocalBuffer();
    void processLocalBuffer();
    void bufferWatcher();
    void createGroupIfNeeded();

    static int                      m_iLastUpdateIndex;
    EVENTSLIB::EventManager*        m_pEventManager;
    QSharedMemory                   m_SharedMemory;
    std::atomic_bool                m_IsInit;
    std::string                     m_sGroupName;
    bool                            m_bGroupCreated;
    idNum                           m_GroupId;
    int                             m_SharedMemorySize;
    int                             m_fTimerCheckBuffer;
    std::thread                     m_BufferWatcherThread;
    std::atomic_bool                m_BufferWatcherThreadRunning;
    std::atomic_bool                m_WritingToSharedMemory;
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

