#include "eventsharedmemmanager.h"

using namespace EVENTSINTERNAL;

static const std::string defaultGroupName("external");
static int sharedMemBufferLength(20);

EventSharedMemManager::EventSharedMemManager()
: m_bSharedMemoryInitState(false)
, m_sGroupName(defaultGroupName)
{

}

