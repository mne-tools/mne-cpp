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

class EventUpdate;
class NewEventUpdate;
class DeleteEventUpdate;

class EventSharedMemManager
{
public:

    EventSharedMemManager(EVENTSLIB::EventManager* parent = nullptr);

    void init(EVENTSLIB::SharedMemoryMode mode);
    void stop();
    bool isInit() const;

    void addEvent(int sample, idNum);
    void deleteEvent(int sample, idNum id);

    static long long getTimeNow();

    void processEvent(NewEventUpdate* ne);

    void processEvent(DeleteEventUpdate* de);

private:

    void storeUpdateEventInBuffer(EventUpdate* nu);
    void bufferWatcher();
    void createEventGroup();

    EVENTSLIB::EventManager*        m_pEventManager;
    QSharedMemory                   m_SharedMemory;
    bool                            m_bIsInit;
    std::string                     m_sGroupName;
    bool                            m_bGroupCreated;
    idNum                           m_GroupId;
    int                             m_iLastUpdateIndex;
    float                           m_fTimerCheckBuffer;
    std::thread                     m_BufferWatcherThread;
    long long                       m_lastCheckTime;
    EventUpdate*                    m_Buffer;
    int                             m_Id;
    enum EVENTSLIB::SharedMemoryMode m_Mode;
};

class EventUpdate
{
public:
    EventUpdate(int sample, idNum id, int creator)
    : m_EventSample(sample)
    , m_EventId(id)
    , m_CreatorId(creator)
    {
        m_CreationTime = EventSharedMemManager::getTimeNow();
    }

    virtual ~EventUpdate() = default;

    virtual void processUpdate(EventSharedMemManager* e) = 0;

    long long creationTime() const
    {
        return m_CreationTime;
    }

    int getSample() const
    {
        return m_EventSample;
    }

    idNum getId() const
    {
        return m_EventId;
    }

    int getCreatorId() const
    {
        return m_CreatorId;
    }
protected:
    int             m_EventSample;
    idNum           m_EventId;
    int             m_CreatorId;
    long long       m_CreationTime;
};

class NewEventUpdate : public EventUpdate
{
public:
    NewEventUpdate(int sample, idNum id, int creator)
        : EventUpdate(sample, id, creator)
    {

    }

    void processUpdate(EventSharedMemManager* e) override
    {
        e->processEvent(this);
    }
};

class DeleteEventUpdate : public EventUpdate
{
public:
    DeleteEventUpdate(int sample, idNum id, int creator)
        : EventUpdate(sample, id, creator)
    {

    }

    void processUpdate(EventSharedMemManager* e) override
    {
        e->processEvent(this);
    }
};


} //namespace
#endif // EVENTSHAREDMEMMANAGER_EVENTS_H


//auto now = std::chrono::system_clock::now();

