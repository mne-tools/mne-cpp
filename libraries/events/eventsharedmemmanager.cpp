#include "eventsharedmemmanager.h"
#include "eventmanager.h"

#include <QDebug>
#include <QString>
#include <utility>

using namespace EVENTSINTERNAL;

static const std::string defaultSharedMemoryBufferKey("MNE_EVENTS_SHAREDMEMORY_BUFFER");
static const std::string defaultGroupName("external");

int EventSharedMemManager::m_iLastUpdateIndex(0);

// The limiting factor in the bandwitdh of the shared memory capabilities of this library
// is measured in terms of buffer length divided by the time interval between checks for updates.
// So, in order to say: The library is capable of correctly handle a
// maximum of "sharedMemBufferLength"/"m_fTimerCheckBuffer" events per second.
constexpr static int bufferLength(5);
static long long defatult_timerBufferWatch(200);

EventUpdate::EventUpdate()
:EventUpdate(0,0,type::NULL_EVENT)
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

std::string EventUpdate::eventTypeToText()
{
    return typeString[m_TypeOfUpdate];
}

EventSharedMemManager::EventSharedMemManager(EVENTSLIB::EventManager* parent)
: m_pEventManager(parent)
, m_SharedMemory(QString::fromStdString(defaultSharedMemoryBufferKey))
, m_IsInit(false)
, m_sGroupName(defaultGroupName)
, m_bGroupCreated(false)
, m_GroupId(0)
, m_SharedMemorySize(sizeof(int) + bufferLength * sizeof(EventUpdate))
, m_fTimerCheckBuffer(defatult_timerBufferWatch)
, m_BufferWatcherThreadRunning(false)
, m_WritingToSharedMemory(false)
, m_lastCheckTime(0)
, m_LocalBuffer(new EventUpdate[bufferLength])
, m_SharedBuffer(nullptr)
, m_Id(generateId())
, m_Mode(EVENTSLIB::SharedMemoryMode::READ)
{

}

EventSharedMemManager::~EventSharedMemManager()
{
    detachFromSharedMemory();
    delete[] m_LocalBuffer;
}
void EventSharedMemManager::init(EVENTSLIB::SharedMemoryMode mode)
{
//    qDebug() << " ========================================================";
//    qDebug() << "Init started!\n";

    if(!m_IsInit)
    {
        detachFromSharedMemory();

        m_Mode = mode;
        if(m_Mode == EVENTSLIB::SharedMemoryMode::READ)
        {
            attachToSharedSegment(QSharedMemory::AccessMode::ReadOnly);
            launchSharedMemoryWatcherThread();

        } else if(m_Mode == EVENTSLIB::SharedMemoryMode::WRITE)
        {
            attachToOrCreateSharedSegment( QSharedMemory::AccessMode::ReadWrite);
        } else if(m_Mode == EVENTSLIB::SharedMemoryMode::READWRITE)
        {
            attachToOrCreateSharedSegment( QSharedMemory::AccessMode::ReadWrite);
            launchSharedMemoryWatcherThread();
        }
    }
}

void EventSharedMemManager::attachToOrCreateSharedSegment(QSharedMemory::AccessMode mode)
{
    attachToSharedSegment(mode);
    if(!m_IsInit)
    {
        m_IsInit = createSharedSegment(m_SharedMemorySize, mode);
    }
}

void EventSharedMemManager::attachToSharedSegment(QSharedMemory::AccessMode mode)
{
    m_IsInit = m_SharedMemory.attach(mode);
    if(m_IsInit)
    {
        m_SharedBuffer = static_cast<EventUpdate*>(m_SharedMemory.data());
    }
}

bool EventSharedMemManager::createSharedSegment(int bufferSize, QSharedMemory::AccessMode mode)
{
    bool output = m_SharedMemory.create(bufferSize, mode);
    if(output)
    {
        m_SharedBuffer = static_cast<EventUpdate*>(m_SharedMemory.data());
        initializeSharedMemory();
    }
    return output;
}

void EventSharedMemManager::launchSharedMemoryWatcherThread()
{
    m_BufferWatcherThread = std::thread(&EventSharedMemManager::bufferWatcher, this);
}

void EventSharedMemManager::detachFromSharedMemory()
{
    stopSharedMemoryWatcherThread();
    if(!m_BufferWatcherThreadRunning && !m_WritingToSharedMemory)
    {
        if(m_SharedMemory.isAttached())
        {
            m_SharedMemory.detach();
        }
    }
}

void EventSharedMemManager::stopSharedMemoryWatcherThread()
{
    if(m_BufferWatcherThreadRunning)
    {
        m_IsInit = false;
        m_BufferWatcherThread.join();
    }
}

void EventSharedMemManager::stop()
{
    detachFromSharedMemory();
    m_IsInit = false;
}

bool EventSharedMemManager::isInit() const
{
    return m_IsInit;
}

void EventSharedMemManager::addEvent(int sample)
{
    if(m_IsInit &&
      (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
       m_Mode == EVENTSLIB::SharedMemoryMode::READWRITE  )  )
    {
        EventUpdate newUpdate(sample, m_Id, EventUpdate::type::NEW_EVENT);
        copyNewUpdateToSharedMemory(newUpdate);
    }
}

void EventSharedMemManager::deleteEvent(int sample)
{
    if(m_IsInit &&
          (m_Mode == EVENTSLIB::SharedMemoryMode::WRITE  ||
           m_Mode == EVENTSLIB::SharedMemoryMode::READWRITE  )  )
    {
        EventUpdate newUpdate(sample, m_Id, EventUpdate::type::DELETE_EVENT);
        copyNewUpdateToSharedMemory(newUpdate);
    }
}

void EventSharedMemManager::initializeSharedMemory()
{
//    qDebug() << "Initializing Shared Memory Buffer ========  id: " << m_Id;
//    printLocalBuffer();
    void* localBuffer = static_cast<void*>(m_LocalBuffer);
    char* sharedBuffer = static_cast<char*>(m_SharedMemory.data()) + sizeof(int);
    int indexIterator(0);
    m_WritingToSharedMemory = true;
    if(m_SharedMemory.isAttached())
    {
        m_SharedMemory.lock();
        memcpy(m_SharedMemory.data(), &indexIterator, sizeof(int));
        memcpy(sharedBuffer, localBuffer, bufferLength * sizeof(EventUpdate));
        m_SharedMemory.unlock();
    }
    m_WritingToSharedMemory = false;
}

void EventSharedMemManager::copyNewUpdateToSharedMemory(EventUpdate& newUpdate)
{
//    qDebug() << "Sending Buffer ========  id: " << m_Id;

    char* sharedBuffer = static_cast<char*>(m_SharedMemory.data()) + sizeof(int);
    int indexIterator(0);
    m_WritingToSharedMemory = true;
    if(m_SharedMemory.isAttached())
    {
        m_SharedMemory.lock();
        memcpy(&indexIterator, m_SharedMemory.data(), sizeof(int));
        memcpy(m_SharedMemory.data(), &(++indexIterator), sizeof(int));
        int index = (indexIterator-1) % bufferLength;
        memcpy(sharedBuffer + (index * sizeof(EventUpdate)), static_cast<void*>(&newUpdate), sizeof(EventUpdate));
        m_SharedMemory.unlock();
    }
    m_WritingToSharedMemory = false;
}

void EventSharedMemManager::copySharedMemoryToLocalBuffer()
{
    void* localBuffer = static_cast<void*>(m_LocalBuffer);
    char* sharedBuffer = static_cast<char*>(m_SharedMemory.data()) + sizeof(int);
    if(m_SharedMemory.isAttached())
    {
        m_SharedMemory.lock();
        memcpy(localBuffer, sharedBuffer, bufferLength * sizeof(EventUpdate));
        m_SharedMemory.unlock();
    }
//    qDebug() << "Receiving Buffer ========  id: " << m_Id;
//    printLocalBuffer();
}

void EventSharedMemManager::bufferWatcher()
{
    m_BufferWatcherThreadRunning = true;
//    qDebug() << "buffer Watcher thread launched";
    while(m_IsInit)
    {
//        qDebug() << "Running buffer watcher!";
        copySharedMemoryToLocalBuffer();
        auto timeCheck = getTimeNow();
        processLocalBuffer();
        m_lastCheckTime = timeCheck;
        std::this_thread::sleep_for(std::chrono::milliseconds(m_fTimerCheckBuffer));
    }
    m_BufferWatcherThreadRunning = false;
}


void EventSharedMemManager::processLocalBuffer()
{
    for(int i = 0; i < bufferLength; ++i)
    {
//        qDebug() << "Checking update: " << i;
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
//    qDebug() << "process new update";
    switch (ne.getType())
    {
        case EventUpdate::type::NEW_EVENT :
        {
            processNewEvent(ne);
            break;
        }
        case EventUpdate::type::DELETE_EVENT :
        {
            processDeleteEvent(ne);
            break;
        }
        default :
            break;
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
        qDebug() << "[" << i << "] -" << m_LocalBuffer[i].eventTypeToText().c_str()
                 << "-" << m_LocalBuffer[i].getSample()
                 << "-" << m_LocalBuffer[i].getCreatorId()
                 << "-" << m_LocalBuffer[i].getCreationTime() << "\n";
    }
}



