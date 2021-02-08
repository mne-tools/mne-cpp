#include "eventmanager.h"

using namespace EVENTSLIB;


//=============================================================================================================

//void EventManager::addEvent(int iSample)
//{
//    Event event(iSample, m_iSelectedGroup);
//    m_eventList.emplace(iSample, m_iSelectedGroup);
//}

////=============================================================================================================

//void EventManager::addGroup(const char *sGroupName)
//{
//    EventGroup eventGroup(sGroupName);
//    m_iSelectedGroup = eventGroup;
//    selectGroup();
//}

std::unique_ptr< std::vector<Event> > EventManager::getAllEvents() const
{
    size_t numEvents(m_eventList.size());
    std::unique_ptr<std::vector<Event> > eventsList(std::make_unique<std::vector<Event> >(numEvents));

    for(const auto& e : m_eventList)
    {
        eventsList->emplace_back(e);
    }
    eventsList->shrink_to_fit();
    return eventsList;
}

std::unique_ptr<std::vector<Group> > EventManager::getAllGroups() const
{
    size_t  numGroups(m_eventGroupList.size());
//    std::unique_ptr<std::vector<Group> > groupsList(std::make_unique<std::vector<Event> >(numGroups));

}

using GroupIterator = std::unordered_map<int, EVENTSINTERNAL::EventGroup>::const_iterator;
