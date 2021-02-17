#include "eventsharedmemmanager.h"
#include "eventmanager.h"

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
: m_pEventManager(parent)
, m_SharedMemory(QString::fromStdString(defaultSharedMemoryKey))
, m_bIsInit(false)
, m_sGroupName(defaultGroupName)
, m_bGroupCreated(false)
, m_GroupId(0)
, m_iLastUpdateIndex(0)
, m_Buffer(nullptr)
, m_Id(rand())
, m_Mode(EVENTSLIB::SharedMemoryMode::READ)
{

}

void EventSharedMemManager::init(EVENTSLIB::SharedMemoryMode mode)
{
    m_Mode = mode;
    m_bIsInit = false;

    if(m_Mode == EVENTSLIB::SharedMemoryMode::READ)
    {
        if(!m_SharedMemory.isAttached())
        {
            m_bIsInit = m_SharedMemory.attach(QSharedMemory::ReadOnly);
            if(m_bIsInit)
            {
                m_Buffer = static_cast<EventUpdate*>(m_SharedMemory.data());
            }
            m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
        }
    } else if(m_Mode == EVENTSLIB::SharedMemoryMode::WRITE)
    {
        if(m_SharedMemory.isAttached())
        {
            stop();
        }
        m_bIsInit = m_SharedMemory.create(
                    sharedMemBufferLength * sizeof(EventUpdate));
        if(m_bIsInit)
        {
            m_Buffer = static_cast<EventUpdate*>(m_SharedMemory.data());
        }
    } else if(m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL)
    {
        if(m_SharedMemory.isAttached())
        {
            stop();
        }
        m_bIsInit = m_SharedMemory.create(
                    sharedMemBufferLength * sizeof(EventUpdate));
        if(m_bIsInit)
        {
            m_Buffer = static_cast<EventUpdate*>(m_SharedMemory.data());
        }
        m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
    }
}

void EventSharedMemManager::stop()
{
    m_SharedMemory.detach();
    m_Buffer = nullptr;
    m_bIsInit = false;
}

bool EventSharedMemManager::isInit() const
{
    return m_bIsInit;
}

void EventSharedMemManager::addEvent(int sample, idNum id)
{
    if(m_bIsInit &&
      (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
       m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL  )  )
    {
        NewEventUpdate newUpdate(sample, id, m_Id);
        storeUpdateEventInBuffer(&newUpdate);
    }
}

void EventSharedMemManager::deleteEvent(int sample, idNum id)
{
    if(m_bIsInit &&
          (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
           m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL  )  )
    {
        DeleteEventUpdate newUpdate(sample, id, m_Id);
        storeUpdateEventInBuffer(&newUpdate);
    }
}

void EventSharedMemManager::storeUpdateEventInBuffer(EventUpdate* newUpdate)
{
    if(m_Buffer)
    {
        m_SharedMemory.lock();
        m_Buffer[m_iLastUpdateIndex%sharedMemBufferLength] = *newUpdate;
        m_SharedMemory.unlock();
        m_iLastUpdateIndex++;
    }
}

void EventSharedMemManager::bufferWatcher()
{
    while(m_bIsInit)
    {
        m_SharedMemory.lock();
        for(int i = 0; i < sharedMemBufferLength; ++i)
        {
            if(m_Buffer[i].creationTime() > m_lastCheckTime &&
               m_Buffer[i].getCreatorId() != m_Id)
            {
                if(!m_bGroupCreated)
                {
                    createEventGroup();
                }
                m_Buffer[i].processUpdate(this);
            }
        }
        m_lastCheckTime = getTimeNow();
        m_SharedMemory.unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(timerBufferWatch));
    }
}

void EventSharedMemManager::processEvent(NewEventUpdate* ne)
{
    m_pEventManager->addEvent(ne->getSample(), m_GroupId);
}

void EventSharedMemManager::processEvent(DeleteEventUpdate* de)
{
    m_pEventManager->deleteEvent(de->getId());
}

long long EventSharedMemManager::getTimeNow()
{
    const auto tNow = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                tNow.time_since_epoch()).count();
}

void EventSharedMemManager::createEventGroup()
{
    EVENTSLIB::EventGroup g = m_pEventManager->addGroup(m_sGroupName);
    m_GroupId = g.id;
    m_bGroupCreated = true;
}
