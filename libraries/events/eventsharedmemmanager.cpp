#include "eventsharedmemmanager.h"

#include <QString>

using namespace EVENTSINTERNAL;

static const std::string defaultSharedMemoryKey("MNE_SHAREDMEMORY_SERVICE");
static const std::string defaultGroupName("external");
static const EVENTSLIB::SharedMemoryMode defaultMemoryAccessMode(EVENTSLIB::SharedMemoryMode::READ);
static int sharedMemBufferLength(20);

EventSharedMemManager::EventSharedMemManager(EVENTSLIB::EventManager* parent)
: m_parent(parent)
, m_SharedMemory(QString::fromStdString(defaultSharedMemoryKey))
, m_bIsInit(false)
, m_MemoryAccessMode(defaultMemoryAccessMode)
, m_sGroupName(defaultGroupName)
{

}

bool EventSharedMemManager::init(EVENTSLIB::SharedMemoryMode mode)
{
    m_bIsInit = false;

    if(mode == EVENTSLIB::SharedMemoryMode::READ)
    {
        if(!m_SharedMemory.isAttached())
        {
            m_bIsInit = m_SharedMemory.attach(QSharedMemory::ReadOnly);

            //launch timer-based reader.

        }
    } else if(mode == EVENTSLIB::SharedMemoryMode::WRITE)
    {
        if(m_SharedMemory.isAttached())
        {
            m_SharedMemory.detach();
            m_bIsInit = false;
        }
        m_bIsInit = m_SharedMemory.create(sharedMemBufferLength * sizeof(EventUpdate));
    }
    return m_bIsInit;
}

bool EventSharedMemManager::stop()
{
    m_SharedMemory.detach();
    m_bIsInit = false;
    return m_bIsInit;
}

bool EventSharedMemManager::isInit() const
{
    return m_bIsInit;
}
