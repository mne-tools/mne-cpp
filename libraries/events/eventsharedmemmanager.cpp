#include "eventsharedmemmanager.h"

#include <QString>
#include <utility>
#include <optional>

using namespace EVENTSINTERNAL;

static const std::string defaultSharedMemoryKey("MNE_SHAREDMEMORY_SERVICE");
static const std::string defaultGroupName("external");

// The limiting factor in the bandwitdh of the shared memory capabilities of this library
// is measured in terms of buffer length divided by the time interval between checks for updates.
// So, in order to say: The library is capable of correctly handle a
// maximum of "sharedMemBufferLength"/"m_fTimerCheckBuffer" events per second.
constexpr static int sharedMemBufferLength(5);
static long long timerBufferWatch(200);

EventSharedMemManager::EventSharedMemManager(EVENTSLIB::EventManager* parent)
: m_parent(parent)
, m_SharedMemory(QString::fromStdString(defaultSharedMemoryKey))
, m_bIsInit(false)
, m_sGroupName(defaultGroupName)
, m_bGroupNotCreated(true)
, m_GroupId(0)
, m_iLastUpdateIndex(0)
, m_Buffer(nullptr)
, m_Id(rand())
{

}

EventSharedMemManager::~EventSharedMemManager()
{
    m_bIsInit = false;
    m_BufferWatcherThread.join();
}

void EventSharedMemManager::init(EVENTSLIB::SharedMemoryMode mode)
{
    m_bIsInit = false;

    if(mode == EVENTSLIB::SharedMemoryMode::READ)
    {
        if(!m_SharedMemory.isAttached())
        {
            m_bIsInit = m_SharedMemory.attach(QSharedMemory::ReadOnly);
            if(m_bIsInit)
            {
                m_Buffer = static_cast<std::unique_ptr<EventUpdate>*>(m_SharedMemory.data());
            }
            m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
        }
    } else if(mode == EVENTSLIB::SharedMemoryMode::WRITE)
    {
        if(m_SharedMemory.isAttached())
        {
            stop();
        }
        m_bIsInit = m_SharedMemory.create(
                    sharedMemBufferLength * sizeof(std::unique_ptr<EventUpdate>));
        if(m_bIsInit)
        {
            m_Buffer = static_cast<std::unique_ptr<EventUpdate>*>(m_SharedMemory.data());
        }
    } else if(mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL)
    {
        if(m_SharedMemory.isAttached())
        {
            stop();
        }
        m_bIsInit = m_SharedMemory.create(
                    sharedMemBufferLength * sizeof(std::unique_ptr<EventUpdate>));
        if(m_bIsInit)
        {
            m_Buffer = static_cast<std::unique_ptr<EventUpdate>*>(m_SharedMemory.data());
        }
        m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
    }
}

void EventSharedMemManager::stop()
{
    m_SharedMemory.detach();
    m_bIsInit = false;
    m_Buffer = nullptr;
    std::terminate();
}

bool EventSharedMemManager::isInit() const
{
    return m_bIsInit;
}

void EventSharedMemManager::addEvent(int sample)
{
    if(m_bIsInit)
    {
        std::unique_ptr<EventUpdate> newUpdate(std::make_unique<NewEventUpdate>(sample, m_Id));
        storeUpdateEventInBuffer(std::move(newUpdate));
    }
}

void EventSharedMemManager::deleteEvent(int sample)
{
    std::unique_ptr<EventUpdate> newUpdate(std::make_unique<DeleteEventUpdate>(sample, m_Id));
    storeUpdateEventInBuffer(std::move(newUpdate));
}

void EventSharedMemManager::storeUpdateEventInBuffer(std::unique_ptr<EventUpdate> newUpdate)
{
    if(m_Buffer)
    {
        m_SharedMemory.lock();
        m_Buffer[m_iLastUpdateIndex%sharedMemBufferLength] = std::move(newUpdate);
        m_SharedMemory.unlock();
        m_iLastUpdateIndex++;
    }
}

void EventSharedMemManager::bufferWatcher()
{
    while(m_bIsInit)
    {
        std::unique_ptr<EventUpdate>* data = static_cast<std::unique_ptr<EventUpdate>*>(m_SharedMemory.data());
        m_SharedMemory.lock();
        for(int i = 0; i < sharedMemBufferLength; ++i)
        {
            if(data[i].get()->creationTime() > m_lastCheckTime &&
               data[i].get()->getCreatorId() != m_Id)
            {
                if(m_bGroupNotCreated)
                {
                    createEventGroup();
                }
                data[i].get()->processUpdate(this);
            }
        }
        m_lastCheckTime = getTimeNow();
        m_SharedMemory.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(timerBufferWatch));
    }
}

void EventSharedMemManager::processEvent(NewEventUpdate* ne)
{


}

void EventSharedMemManager::processEvent(DeleteEventUpdate* de)
{

}

long long EventSharedMemManager::getTimeNow()
{
    const auto tNow = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                tNow.time_since_epoch()).count();
}

void EventSharedMemManager::createEventGroup()
{
    m_GroupId = m_parent->
}
