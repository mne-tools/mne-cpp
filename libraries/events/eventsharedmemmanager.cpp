#include "eventsharedmemmanager.h"
#include "eventmanager.h"

#include <QDebug>
#include <QString>
#include <utility>

using namespace EVENTSINTERNAL;

static const std::string defaultSharedMemoryKey("MNE_SHAREDMEMORY_S");
static const std::string defaultGroupName("external");

int EventSharedMemManager::m_iLastUpdateIndex(0);

// The limiting factor in the bandwitdh of the shared memory capabilities of this library
// is measured in terms of buffer length divided by the time interval between checks for updates.
// So, in order to say: The library is capable of correctly handle a
// maximum of "sharedMemBufferLength"/"m_fTimerCheckBuffer" events per second.
constexpr static int bufferLength(1);
static long long timerBufferWatch(4000);


EventUpdate::EventUpdate()
:EventUpdate(0,0,type::NewEvent)
{ }

EventUpdate::EventUpdate(int sample, int creator,type t)
: m_EventSample(sample)
, m_CreatorId(creator)
, m_TypeOfUpdate(t)
{
    m_CreationTime = EventSharedMemManager::getTimeNow();
}

long long EventUpdate::getCreationTime() const
{
    return m_CreationTime;
}

int EventUpdate::getSample() const
{
    return m_EventSample;
}

int EventUpdate::getCreatorId() const
{
    return m_CreatorId;
}

EventUpdate::type EventUpdate::getType() const
{
    return m_TypeOfUpdate;
}

void EventUpdate::setType(type t)
{
    m_TypeOfUpdate = t;
}

EventSharedMemManager::EventSharedMemManager(EVENTSLIB::EventManager* parent)
: m_pEventManager(parent)
, m_SharedMemory(QString::fromStdString(defaultSharedMemoryKey))
, m_bIsInit(false)
, m_sGroupName(defaultGroupName)
, m_bGroupCreated(false)
, m_GroupId()
, m_LocalBuffer(new EventUpdate[bufferLength])
, m_SharedBuffer(nullptr)
, m_Id(generateId())
, m_Mode(EVENTSLIB::SharedMemoryMode::READ)
{

}

EventSharedMemManager::~EventSharedMemManager()
{
    stop();
    delete[] m_LocalBuffer;
}
void EventSharedMemManager::init(EVENTSLIB::SharedMemoryMode mode)
{
    qDebug() << " ========================================================";
    qDebug() << "Init started!       !!! \n";

    if(!m_bIsInit)
    {
        m_Mode = mode;
        if(m_Mode == EVENTSLIB::SharedMemoryMode::READ)
        {
            if(!m_SharedMemory.isAttached())
            {
                m_bIsInit = m_SharedMemory.attach(QSharedMemory::ReadOnly);
                if(m_bIsInit)
                {
                    m_SharedBuffer = static_cast<EventUpdate*>(m_SharedMemory.data());
                }
                m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
            }
        } else if(m_Mode == EVENTSLIB::SharedMemoryMode::WRITE)
        {
            if(m_SharedMemory.isAttached())
            {
                m_SharedMemory.detach();
            }
            m_bIsInit = m_SharedMemory.create(
                        bufferLength * sizeof(EventUpdate),
                        QSharedMemory::AccessMode::ReadWrite);

            if(m_bIsInit)
            {
                m_SharedBuffer = static_cast<EventUpdate*>(m_SharedMemory.data());
            }

        } else if(m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL)
        {
            qDebug() << "Bydirectional mode!\n";
            if(m_SharedMemory.isAttached())
            {
                m_SharedMemory.detach();
            }

            m_bIsInit = m_SharedMemory.attach(QSharedMemory::AccessMode::ReadWrite);
            qDebug() << "Checking for attach. m_bIsInit = " << m_bIsInit;

            if(!m_bIsInit)
            {
                m_bIsInit = m_SharedMemory.create(
                            bufferLength * sizeof(EventUpdate),
                            QSharedMemory::AccessMode::ReadWrite);
                qDebug() << "Creating segment: " << m_bIsInit;
            }


            if(m_bIsInit)
            {
                m_SharedBuffer = static_cast<EventUpdate*>(m_SharedMemory.data());
                qDebug() << " m_Buffer " << m_SharedBuffer;
                m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
            }
        }
    }
}

void EventSharedMemManager::stop()
{
    m_bIsInit = false;
    m_BufferWatcherThread.join();
    if(m_SharedMemory.isAttached())
    {
        m_SharedMemory.detach();
    }
}

bool EventSharedMemManager::isInit() const
{
    return m_bIsInit;
}

void EventSharedMemManager::addEvent(int sample)
{
    if(m_bIsInit &&
      (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
       m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL  )  )
    {
        EventUpdate newUpdate(sample, m_Id, EventUpdate::type::NewEvent);
        storeUpdateInSharedMemory(newUpdate);
    }
}

void EventSharedMemManager::deleteEvent(int sample)
{
    if(m_bIsInit &&
          (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
           m_Mode == EVENTSLIB::SharedMemoryMode::BYDIRECTIONAL  )  )
    {
        EventUpdate newUpdate(sample, m_Id, EventUpdate::type::DeleteEvent);
        storeUpdateInSharedMemory(newUpdate);
    }
}

void EventSharedMemManager::storeUpdateInSharedMemory(const EventUpdate& newUpdate)
{
    m_LocalBuffer[m_iLastUpdateIndex % bufferLength] = newUpdate;
    m_iLastUpdateIndex++;
    copyLocalBufferToSharedMemory();
}

void EventSharedMemManager::copyLocalBufferToSharedMemory()
{
    qDebug() << "Sending Buffer ========  id: " << m_Id;
    printLocalBuffer();
    void* localBuffer = static_cast<void*>(m_LocalBuffer);
    m_SharedMemory.lock();
    memcpy(m_SharedMemory.data(),localBuffer,bufferLength * sizeof(EventUpdate));
    m_SharedMemory.unlock();

}

void EventSharedMemManager::copySharedMemoryToLocalBuffer()
{
    void* localBuffer = static_cast<void*>(m_LocalBuffer);
    m_SharedMemory.lock();
    memcpy(localBuffer, m_SharedBuffer,bufferLength * sizeof(EventUpdate));
    m_SharedMemory.unlock();
    qDebug() << "Receiving Buffer ========  id: " << m_Id;
    printLocalBuffer();
}

void EventSharedMemManager::bufferWatcher()
{
    qDebug() << "buffer Watcher thread launched";
    while(m_bIsInit)
    {
        qDebug() << "Running buffer watcher!";
        copySharedMemoryToLocalBuffer();
        auto timeCheck = getTimeNow();

        processLocalBuffer();

        m_lastCheckTime = timeCheck;
        std::this_thread::sleep_for(std::chrono::milliseconds(timerBufferWatch));
    }
}


void EventSharedMemManager::processLocalBuffer()
{
    for(int i = 0; i < bufferLength; ++i)
    {
        qDebug() << "Checking update: " << i;
        if(m_LocalBuffer[i].getCreationTime() > m_lastCheckTime &&
           m_LocalBuffer[i].getCreatorId() != m_Id )
        {
            createGroupIfNeeded();
            processEvent(m_LocalBuffer[i]);
        }
    }
}

void EventSharedMemManager::processEvent(const EventUpdate& ne)
{
    qDebug() << "process event new update";

    switch (ne.getType())
    {
        case EventUpdate::type::NewEvent :
        {
            processNewEvent(ne);
            break;
        }
        case EventUpdate::type::DeleteEvent :
        {
            processDeleteEvent(ne);
            break;
        }
    }
}

void EventSharedMemManager::processNewEvent(const EventUpdate& ne)
{
    EVENTSINTERNAL::EventINT newEvent(
                m_pEventManager->generateNewEventId(), ne.getSample(), m_GroupId);
    m_pEventManager->insertEvent(newEvent);
}

void EventSharedMemManager::processDeleteEvent(const EventUpdate& ne)
{
    auto eventsInSample = m_pEventManager->getEventsInSample(ne.getSample());
    for(auto e: *eventsInSample)
    {
        if(e.groupId == m_GroupId)
        {
            m_pEventManager->eraseEvent(e.id);
            break;
        };
    }
}

long long EventSharedMemManager::getTimeNow()
{
    const auto tNow = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
                tNow.time_since_epoch()).count();
}

void EventSharedMemManager::createGroupIfNeeded()
{
    if(!m_bGroupCreated)
    {
        EVENTSLIB::EventGroup g = m_pEventManager->addGroup(m_sGroupName);
        m_GroupId = g.id;
        m_bGroupCreated = true;
    }
}

void EventSharedMemManager::printLocalBuffer()
{
    for(int i = 0; i < bufferLength; ++i)
    {
        qDebug() << "[" << i << "] -"<< EventTypeToText(m_LocalBuffer[i].getType()).c_str()
                 << "-" << m_LocalBuffer[i].getSample()
                 << "-" << m_LocalBuffer[i].getCreatorId()
                 << "-" << m_LocalBuffer[i].getCreationTime() << "\n";
    }
}

std::string EventSharedMemManager::EventTypeToText(EventUpdate::type t)
{
    switch (t)
    {
    case EventUpdate::type::NewEvent:
        return "New Event";
    case EventUpdate::type::DeleteEvent:
        return "Delete Event";
    default:
        return "DUNNO";
    }
}

