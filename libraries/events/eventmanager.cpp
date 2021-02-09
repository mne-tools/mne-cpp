#include "eventmanager.h"

using namespace EVENTSLIB;

constexpr idNum eventIdCounter(0);
constexpr idNum groupIdCounter(0);

EventManager::EventManager()
{

}

//=============================================================================================================

int EventManager::getNumEvents() const
{
    return m_EventList.size();
}

int EventManager::getNumGroups() const
{
    return m_pGroupList.size();
}

//=============================================================================================================

Event EventManager::getEvent(idNum eventId) const
{
    for( std::multiset<EVENTSINTERNAL::Event>::const_iterator itr = m_EventList.begin();
         itr != m_EventList.end(); itr++)
    {
        if(itr->getId() == eventId)
        {
            return Event(*itr);
        }
    }

    return Event();
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEvents(const std::vector<idNum> eventIds) const noexcept
{
    std::unique_ptr<std::vector<Event> > pEventList(std::make_unique<std::vector<Event> >());

    for(std::vector<idNum>::const_iterator eventIdsIter = eventIds.begin();
        eventIdsIter != eventIds.end(); eventIdsIter++)
    {
        for( std::multiset<EVENTSINTERNAL::Event>::const_iterator itr = m_EventList.begin();
             itr != m_EventList.end(); itr++)
        {
            if(*eventIdsIter == itr->getId())
            {
                pEventList->emplace_back(Event(*itr));
            }
        }
    }
    return pEventList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getAllEvents() const noexcept
{
    std::unique_ptr<std::vector<Event> > pEventList(std::make_unique<std::vector<Event> >(getNumEvents()));
    for(const EVENTSINTERNAL::Event& e: m_EventList)
    {
        pEventList->emplace_back(Event(e));
    }

    return pEventList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEventsInSample(int sample) const noexcept
{
    EVENTSINTERNAL::Event e(0,sample, 0);
    int numEvents = m_EventList.count(e);
    std::unique_ptr<std::vector<Event> > pEventList(std::make_unique<std::vector<Event> >(numEvents));
    auto eventsFound = m_EventList.find(e);

    for(int i = 0; i < numEvents; ++i)
    {
        pEventList->emplace_back(Event(*eventsFound));
        eventsFound++;
    }

    return pEventList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEventsBetween(int sampleStart, int sampleEnd) const noexcept
{
    int memoryHint = (sampleEnd-sampleStart)/100;
    std::unique_ptr<std::vector<Event> > pEventList(std::make_unique<std::vector<Event> >(memoryHint));

    auto itr = m_EventList.lower_bound(EVENTSINTERNAL::Event(0, sampleStart, 0));
    auto itrEnd = m_EventList.upper_bound(EVENTSINTERNAL::Event(0, sampleEnd, 0));

    for(; itr != itrEnd; itr++)
    {
        pEventList->emplace_back(Event(*itr));
    }
    pEventList->shrink_to_fit();
    return pEventList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEventsInGroup(const Group& group) const noexcept
{
    return getEventsInGroup(group.id);
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEventsInGroup(const idNum groupId) const noexcept
{
    std::unique_ptr<std::vector<Event> > pEventList(std::make_unique<std::vector<Event> >());
    for(const auto& e: m_EventList)
    {
        if(e.getGroup() == groupId)
        {
            pEventList->emplace_back(Event(e));
        }
    }

    pEventList->shrink_to_fit();
    return pEventList;
}















//=============================================================================================================

Event EventManager::addEvent(int iSample, idNum groupId)
{
    EVENTSINTERNAL::Event e(generateNewEventId(), iSample, groupId);
    m_EventList.insert(e);
    return Event(e);
}

//=============================================================================================================

Event EventManager::moveEvent(idNum eventId, int newSample)
{

}



//

//=============================================================================================================

//void EventManager::addGroup(const char *sGroupName)
//{
//    EventGroup eventGroup(sGroupName);
//    m_iSelectedGroup = eventGroup;
//    selectGroup();
//}


std::unique_ptr<std::vector<Group> > EventManager::getAllGroups() const
{
    size_t  numGroups(m_pGroupList.size());
//    std::unique_ptr<std::vector<Group> > groupsList(std::make_unique<std::vector<Event> >(numGroups));

}


idNum EventManager::generateNewEventId() const
{
    return ++eventIdCounter;
}

idNum EventManager::generateNewGroupId() const
{
    return ++groupIdCounter;
}
