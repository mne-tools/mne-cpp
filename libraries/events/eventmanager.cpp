#include "eventmanager.h"

using namespace EVENTSLIB;

idNum EventManager::eventIdCounter(0);
idNum EventManager::groupIdCounter(0);

EventManager::EventManager()
{

}

//=============================================================================================================

auto EventManager::findEventINT(idNum eventId) const
{
    int sample = m_MapIdToSample.at(eventId);
    auto eventsRange = m_EventsListBySample.equal_range(sample);
    for(auto e = eventsRange.first; e != eventsRange.second; ++e)
    {
        if( e->second.getId() == eventId)
        {
            return e;
        }
    }
    return m_EventsListBySample.end();
}

//=============================================================================================================

idNum EventManager::generateNewEventId() const
{
    return ++eventIdCounter;
}

//=============================================================================================================

idNum EventManager::generateNewGroupId() const
{
    return ++groupIdCounter;
}

//=============================================================================================================

int EventManager::getNumEvents() const
{
    return m_EventsListBySample.size();
}

//=============================================================================================================

Event EventManager::getEvent(idNum eventId) const
{
//    int sample = m_MapIdToSample.at(eventId);
//    auto eventsRange = m_EventsListBySample.equal_range(sample);
//    for(auto e = eventsRange.first; e != eventsRange.second; ++e)
//    {
//        if( e->second.getId() == eventId)
//        {
//            return Event(e->second);
//        }
//    }
    return findEventINT(eventId)->second;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> >
EventManager::getEvents(const std::vector<idNum> eventIds) const
{
    auto pEventsList(allocateOutputContainer<Event>(eventIds.size()));
    for (const auto& id: eventIds)
    {
        pEventsList->push_back(getEvent(id));
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getAllEvents() const
{
    auto pEventsList(allocateOutputContainer<Event>(getNumEvents()));
    for(auto& e: m_EventsListBySample)
    {
        pEventsList->emplace_back(Event(e.second));
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> > EventManager::getEventsInSample(int sample) const
{
    int numEventsInSample = m_EventsListBySample.count(sample);
    auto pEventsList(allocateOutputContainer<Event>(numEventsInSample));

    auto eventsRange = m_EventsListBySample.equal_range(sample);
    for(auto e = eventsRange.first; e != eventsRange.second; e++)
    {
        pEventsList->emplace_back(Event(e->second));
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> >
EventManager::getEventsBetween(int sampleStart, int sampleEnd) const
{
    int memoryHint = ((sampleEnd-sampleStart)/1000)+2;
    auto pEventsList(allocateOutputContainer<Event>(memoryHint));

    auto eventStart = m_EventsListBySample.lower_bound(sampleStart);
    auto eventEnd = m_EventsListBySample.upper_bound(sampleEnd);

    for(auto e = eventStart; e != eventEnd; e++)
    {
        pEventsList->emplace_back(Event(e->second));
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> >
EventManager::getEventsBetween(int sampleStart, int sampleEnd, idNum groupId) const
{
    int memoryHint = ((sampleEnd-sampleStart)/1000)+2;
    auto pEventsList(allocateOutputContainer<Event>(memoryHint));

    auto eventStart = m_EventsListBySample.lower_bound(sampleStart);
    auto eventEnd = m_EventsListBySample.upper_bound(sampleEnd);

    for(auto e = eventStart; e != eventEnd; e++)
    {
        if(e->second.getGroupId() == groupId)
        {
            pEventsList->emplace_back(Event(e->second));
        }
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> >
EventManager::getEventsBetween(int sampleStart, int sampleEnd, const std::vector<idNum>& groupIdsList) const
{
    int memoryHint = (sampleEnd-sampleStart)/200;
    auto pEventsList(allocateOutputContainer<Event>(memoryHint));


    auto eventStart = m_EventsListBySample.lower_bound(sampleStart);
    auto eventEnd = m_EventsListBySample.upper_bound(sampleEnd);

    for(auto e = eventStart; e != eventEnd; e++)
    {
        for(auto& groupId: groupIdsList)
        {
            if(e->second.getGroupId() == groupId)
            {
                pEventsList->emplace_back(Event(e->second));
            }
        }
    }
    return pEventsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<Event> >
EventManager::getEventsInGroup(const idNum groupId) const
{
    auto pEventsList(allocateOutputContainer<Event>());

    for(const auto& e: m_EventsListBySample)
    {
        if(e.second.getGroupId() == groupId)
        {
            pEventsList->emplace_back(Event(e.second));
        }
    }
    return pEventsList;
}

//=============================================================================================================

Event EventManager::addEvent(int sample, idNum groupId)
{
    EVENTSINTERNAL::EventINT newEvent(generateNewEventId(), sample, groupId);
    insertEvent(newEvent);
    return Event(newEvent);
}

//=============================================================================================================

void EventManager::deleteEvent(idNum eventId) noexcept
{
    m_EventsListBySample.erase(findEventINT(eventId));
    m_MapIdToSample.erase(eventId);
}

//=============================================================================================================

void EventManager::deleteEvents(const std::vector<idNum>& eventIds)
{
    for(const auto& id: eventIds)
    {
        deleteEvent(id);
    }
}

//=============================================================================================================

Event EventManager::moveEvent(idNum eventId, int newSample)
{
    auto e = findEventINT(eventId);
    EVENTSINTERNAL::EventINT newEvent(e->second);
    newEvent.setSample(newSample);
    deleteEvent(eventId);
    insertEvent(newEvent);
    return newEvent;
}

//=============================================================================================================

void EventManager::deleteEventsInGroup(idNum groupId)
{
    std::vector<idNum> idList;
    for(auto& e: m_EventsListBySample)
    {
        if(e.second.getGroupId() == groupId)
        {
            idList.emplace_back(e.second.getId());
        }
    }
    deleteEvents(idList);
}

//=============================================================================================================

void EventManager::insertEvent(const EVENTSINTERNAL::EventINT& e)
{
    m_EventsListBySample.emplace(std::make_pair(e.getSample(),e));
    m_MapIdToSample[e.getId()] = e.getSample();
}

//=============================================================================================================

int EventManager::getNumGroups() const
{
    return m_GroupsList.size();
}

//=============================================================================================================

EventGroup EventManager::getGroup(const idNum groupId) const
{
    auto groupFound = m_GroupsList.find(groupId);
    if(groupFound != m_GroupsList.end())
    {
        return EventGroup(groupFound->second);
    } else
    {
        return EventGroup();
    }
}

//=============================================================================================================

std::unique_ptr<std::vector<EventGroup> > EventManager::getAllGroups() const
{
    size_t  numGroups(m_GroupsList.size());
    auto pGroupsList(allocateOutputContainer<EventGroup>(numGroups));
    for(const auto& g: m_GroupsList)
    {
        pGroupsList->emplace_back(EventGroup(g.second));
    }
    return pGroupsList;
}

//=============================================================================================================

std::unique_ptr<std::vector<EventGroup> >
EventManager::getGroups(const std::vector<idNum>& groupIds) const
{
    auto pGroupList(allocateOutputContainer<EventGroup>(groupIds.size()));
    for(const auto& id: groupIds)
    {
        pGroupList->push_back(getGroup(id));
    }
    return pGroupList;
}

//=============================================================================================================

EventGroup EventManager::addGroup(const std::string& sGroupName)
{
    EVENTSINTERNAL::EventGroupINT newGroup(generateNewGroupId(), sGroupName);
    m_GroupsList.emplace(newGroup.getId(), newGroup);
    return EventGroup(newGroup);
}

//=============================================================================================================

EventGroup EventManager::addGroup(const std::string& sGroupName, const RgbColor& color)
{
    EVENTSINTERNAL::EventGroupINT newGroup(generateNewGroupId(), sGroupName, color);
    m_GroupsList.emplace(newGroup.getId(), newGroup);
    return EventGroup(newGroup);
}

//=============================================================================================================

void EventManager::deleteGroup(const idNum groupId)
{
    auto events = getEventsInGroup(groupId);
    if(events->empty())
    {
        auto groupToDelete = m_GroupsList.find(groupId);
        m_GroupsList.erase(groupToDelete);
    }
}

//=============================================================================================================

void EventManager::deleteGroups(const std::vector<idNum>& groupIds)
{
    for(auto g: groupIds)
    {
        deleteGroup(g);
    }
}

//=============================================================================================================

EventGroup EventManager::renameGroup(const idNum groupId, const std::string& newName)
{
    auto group = m_GroupsList.find(groupId);
    if(group != m_GroupsList.end())
    {
        group->second.setName(newName);
    }
    return EventGroup(group->second);
}

EventGroup EventManager::setGroupColor(const idNum groupId, const RgbColor& color)
{
    auto group = m_GroupsList.find(groupId);
    if( group != m_GroupsList.end())
    {
        group->second.setColor(color);
    }
    return EventGroup(group->second);
}

//=============================================================================================================

EventGroup EventManager::mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName)
{
    EVENTSLIB::EventGroup newGroup = addGroup(newName);
    auto eventsAll = getAllEvents();
    for(const auto& ev: *eventsAll)
    {
        for(auto g: groupIds)
        {
            if(ev.groupId == g)
            {
                addEventToGroup(ev.id, newGroup.id);
            }
        }
    }
    deleteGroups(groupIds);
    return newGroup;
}

//=============================================================================================================

EventGroup EventManager::duplicateGroup(const idNum groupId, const std::string& newName)
{
    EVENTSLIB::EventGroup newGroup = addGroup(newName);
    auto eventsToDuplicate = getEventsInGroup(groupId);
    for( const auto& e: *eventsToDuplicate)
    {
        addEvent(e.sample, newGroup.id);
    }
    return newGroup;
}

//=============================================================================================================

void EventManager::addEventToGroup(const idNum eventId, const idNum groupId)
{
    int sample = m_MapIdToSample.at(eventId);
    auto eventsRange = m_EventsListBySample.equal_range(sample);
    std::multimap<int, EVENTSINTERNAL::EventINT>::iterator e = eventsRange.first;
    for(; e != eventsRange.second; ++e)
    {
        if( e->second.getId() == eventId)
        {
            e->second.setGroupId(groupId);
            break;
        }
    }
}
