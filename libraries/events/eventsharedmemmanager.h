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

    ~EventSharedMemManager();

    void init(EVENTSLIB::SharedMemoryMode mode);
    void stop();
    bool isInit() const;

    void addEvent(int sample, idNum);
    void deleteEvent(int sample, idNum id);

    static long long getTimeNow();

    void processEvent(NewEventUpdate* ne);

    void processEvent(DeleteEventUpdate* de);

private:

    void storeUpdateEventInBuffer(std::unique_ptr<EventUpdate> nu);
    void bufferWatcher();
    void createEventGroup();

    EVENTSLIB::EventManager*        m_parent;
    QSharedMemory                   m_SharedMemory;
    bool                            m_bIsInit;
    std::string                     m_sGroupName;
    bool                            m_bGroupCreated;
    idNum                           m_GroupId;
    int                             m_iLastUpdateIndex;
    float                           m_fTimerCheckBuffer;
    std::thread                     m_BufferWatcherThread;
    long long                       m_lastCheckTime;
    std::unique_ptr<EventUpdate>*   m_Buffer;
    int                             m_Id;
};

class EventUpdate
{
public:
    EventUpdate(int sample, idNum id, int creator)
        : mEventSample(sample)
        , mEventId(id)
        , mCreatorId(creator)
    {
        mCreationTime = EventSharedMemManager::getTimeNow();
    }

    virtual ~EventUpdate() = default;

    virtual void processUpdate(EventSharedMemManager* e) = 0;

    long long creationTime() const
    {
        return mCreationTime;
    }

    int getSample() const
    {
        return mEventSample;
    }

    idNum getId() const
    {
        return mEventId;
    }

    int getCreatorId() const
    {
        return mCreatorId;
    }
protected:
    int             mEventSample;
    idNum           mEventId;
    int             mCreatorId;
    long long       mCreationTime;
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

