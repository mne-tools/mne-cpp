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

    void addEvent(int sample);
    void deleteEvent(int sample);

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
    bool                            m_bGroupNotCreated;
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
    EventUpdate(int sample, int creator)
        : mSample(sample)
        , mCreatorId(creator)
    {
        mCreationTime = EventSharedMemManager::getTimeNow();
    };
    virtual ~EventUpdate();

    virtual void processUpdate(EventSharedMemManager* e) = 0;

    long long creationTime() const
    {
        return mCreationTime;
    }

    int getCreatorId() const
    {
        return mCreatorId;
    }
protected:
    int             mSample;
    int             mCreatorId;
    long long       mCreationTime;
};

class NewEventUpdate : public EventUpdate
{
    NewEventUpdate(int sample, int creator)
        : EventUpdate(sample, creator)
    {

    }

    void processUpdate(EventSharedMemManager* e) override
    {
        e->processEvent(this);
    }
};

class DeleteEventUpdate : public EventUpdate
{
    DeleteEventUpdate(int sample, int creator)
        : EventUpdate(sample, creator)
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

