#include "events.h"

using namespace EVENTSLIB;
using namespace EVENTSINTERNAL;

EventManager::EventManager()
:m_iSelectedGroup("Default")
{
    saveSelectedGroup();
}

//=============================================================================================================

void EventManager::saveSelectedGroup()
{
    m_eventGroupList.insert({m_iSelectedGroup.getId(), m_iSelectedGroup});
}

//=============================================================================================================

void EventManager::addEvent(int iSample)
{
    Event event(iSample, m_iSelectedGroup);
    m_eventList.emplace(iSample, m_iSelectedGroup);
}

//=============================================================================================================

void EventManager::addGroup(const char *sGroupName)
{
    EventGroup eventGroup(sGroupName);
    m_iSelectedGroup = eventGroup;
    saveSelectedGroup();
}
