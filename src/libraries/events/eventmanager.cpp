//=============================================================================================================
/**
 * @file     eventgroup.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.8
 * @date     February, 2021
 *
 * @section  LICENSE
 *
 * Copyright (C) 2021, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief     EventManager definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventmanager.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EVENTSLIB;

//=============================================================================================================
// local and static declarations
//=============================================================================================================

constexpr static int invalidID(0);              /**< A variable storing an id value which will never be used, by design.*/
static std::string defaultGroupName("Default"); /**< A name to be used as the name of the default group of events. */

//=============================================================================================================

EventManager::EventManager()
: m_iEventIdCounter(invalidID)
, m_iGroupIdCounter(invalidID)
, m_bDefaultGroupNotCreated(true)
, m_DefaultGroupId(invalidID)
{
#ifndef NO_IPC
    m_pSharedMemManager = std::make_unique<EVENTSINTERNAL::EventSharedMemManager>(this);
#endif
}

//=============================================================================================================

Event EventManager::getEvent(idNum eventId) const
{
    auto eventInt = findEventINT(eventId);
    if(eventInt != m_EventsListBySample.end())
    {
        return Event(eventInt->second);
    }
    return {};
}

//=============================================================================================================

std::multimap<int, EVENTSINTERNAL::EventINT>::const_iterator EventManager::findEventINT(idNum eventId) const
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

std::unique_ptr<std::vector<Event> >
EventManager::getEvents(const std::vector<idNum> eventIds) const
{
    auto pEventsList(allocateOutputContainer<Event>(eventIds.size()));
    for (const auto& id: eventIds)
    {
        auto event = getEvent(id);
        if(event.id != invalidID)
        {
            pEventsList->push_back(event);
        }
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
    size_t numEventsInSample = m_EventsListBySample.count(sample);
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

std::unique_ptr<std::vector<Event> > EventManager::getEventsInGroups(const std::vector<idNum>& groupIdsList) const
{
    auto pEventsList(allocateOutputContainer<Event>());

    for(const auto& e: m_EventsListBySample)
    {
        for (const auto groupId : groupIdsList)
        {
            if(e.second.getGroupId() == groupId)
            {
                pEventsList->emplace_back(Event(e.second));
                break;
            }
        }
    }

    return pEventsList;
}

//=============================================================================================================

idNum EventManager::generateNewEventId()
{
    return ++m_iEventIdCounter;
}

//=============================================================================================================

idNum EventManager::generateNewGroupId()
{
    return ++m_iGroupIdCounter;
}

//=============================================================================================================

size_t EventManager::getNumEvents() const
{
    return m_EventsListBySample.size();
}

//=============================================================================================================

Event EventManager::addEvent(int sample, idNum groupId)
{
    EVENTSINTERNAL::EventINT newEvent(generateNewEventId(), sample, groupId);
    insertEvent(newEvent);

    if(m_pSharedMemManager->isInit())
    {
        qDebug() << "Sending event to SM: Sample: " << sample;
        m_pSharedMemManager->addEvent(newEvent.getSample());
    }

    return Event(newEvent);
}

//=============================================================================================================

Event EventManager::addEvent(int sample)
{
    createDefaultGroupIfNeeded();
    return addEvent(sample, m_DefaultGroupId);
}

//=============================================================================================================

bool EventManager::moveEvent(idNum eventId, int newSample)
{
    bool status(false);
    auto event = findEventINT(eventId);
    if(event != m_EventsListBySample.end())
    {
        EVENTSINTERNAL::EventINT newEvent(event->second);
        newEvent.setSample(newSample);
        deleteEvent(eventId);
        insertEvent(newEvent);
        status = true;
    }
    return status;
}

//=============================================================================================================

bool EventManager::deleteEvent(idNum eventId) noexcept
{
    bool eventFound(false);
    eventFound = eraseEvent(eventId);
    if(eventFound && m_pSharedMemManager->isInit())
    {
        m_pSharedMemManager->deleteEvent(m_MapIdToSample.at(eventId));
    }
    return eventFound;
}

//=============================================================================================================

bool EventManager::eraseEvent(idNum eventId)
{
    auto event = findEventINT(eventId);
    if(event != m_EventsListBySample.end())
    {
        m_EventsListBySample.erase(event);
        m_MapIdToSample.erase(eventId);
        return true;
    }
    return false;
}

//=============================================================================================================

bool EventManager::deleteEvents(const std::vector<idNum>& eventIds)
{
    bool status(eventIds.size());
    for(const auto& id: eventIds)
    {
        status = status && deleteEvent(id);
    }
    return status;
}

//=============================================================================================================

bool EventManager::deleteEvents(std::unique_ptr<std::vector<Event> > eventIds)
{
    bool status(eventIds->size());
    for(auto e: *eventIds){
        status = status && deleteEvent(e.id);
    }
    return status;
}

//=============================================================================================================

bool EventManager::deleteEventsInGroup(idNum groupId)
{
    std::vector<idNum> idList;
    for(auto& e: m_EventsListBySample)
    {
        if(e.second.getGroupId() == groupId)
        {
            idList.emplace_back(e.second.getId());
        }
    }
    return deleteEvents(idList);
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
    return (int)(m_GroupsList.size());
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
        return {};
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
        auto group = getGroup(id);
        if (group.id != invalidID)
        {
            pGroupList->push_back(group);
        }
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

bool EventManager::deleteGroup(const idNum groupId)
{
    bool out(false);
    auto events = getEventsInGroup(groupId);
    if(events->empty())
    {
        auto groupToDeleteIter = m_GroupsList.find(groupId);
        if(groupToDeleteIter != m_GroupsList.end())
        {
            m_GroupsList.erase(groupToDeleteIter);
            out = true;
            if(!m_bDefaultGroupNotCreated && (groupId == m_DefaultGroupId))
            {
                m_bDefaultGroupNotCreated = true;
                m_DefaultGroupId = invalidID;
            }
        }
    }
    return out;
}

//=============================================================================================================

bool EventManager::deleteGroups(const std::vector<idNum>& groupIds)
{
    bool out(groupIds.size());
    for(auto g: groupIds)
    {
        out = out && deleteGroup(g);
    }
    return out;
}

//=============================================================================================================

void EventManager::renameGroup(const idNum groupId, const std::string& newName)
{
    auto group = m_GroupsList.find(groupId);
    if(group != m_GroupsList.end())
    {
        group->second.setName(newName);
    }
}

//=============================================================================================================

void EventManager::setGroupColor(const idNum groupId, const RgbColor& color)
{
    auto group = m_GroupsList.find(groupId);
    if( group != m_GroupsList.end())
    {
        group->second.setColor(color);
    }
}

//=============================================================================================================

EventGroup EventManager::mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName)
{
    EVENTSLIB::EventGroup newGroup = addGroup(newName);
    auto eventsAll = getAllEvents();
    bool state(true);
    for(const auto& ev: *eventsAll)
    {
        for(auto g: groupIds)
        {
            if(ev.groupId == g)
            {
                state = state && addEventToGroup(ev.id, newGroup.id);
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

bool EventManager::addEventToGroup(const idNum eventId, const idNum groupId)
{
    bool state(false);
    int sample(0);
    if(m_MapIdToSample.count(eventId))
    {
        sample = m_MapIdToSample.at(eventId);
        auto eventsRange = m_EventsListBySample.equal_range(sample);
        std::multimap<int, EVENTSINTERNAL::EventINT>::iterator e = eventsRange.first;
        for(; e != eventsRange.second; ++e)
        {
            if( e->second.getId() == eventId)
            {
                e->second.setGroupId(groupId);
                state = true;
                break;
            }
        }
    }
    return state;
}

//=============================================================================================================

bool EventManager::addEventsToGroup(const std::vector<idNum>& eventIds, const idNum groupId)
{
    bool state(true);
    for( idNum id: eventIds)
    {
        state = state && addEventToGroup(id, groupId);
    }
    return state;
}

//=============================================================================================================

void EventManager::initSharedMemory()
{
#ifndef NO_IPC
    initSharedMemory(SharedMemoryMode::READ);
#endif
}

//=============================================================================================================

void EventManager::initSharedMemory(SharedMemoryMode mode)
{
#ifndef NO_IPC
    m_pSharedMemManager->init(mode);
#endif
}

//=============================================================================================================

void EventManager::stopSharedMemory()
{
#ifndef NO_IPC
    m_pSharedMemManager->stop();
#endif
}

//=============================================================================================================

bool EventManager::isSharedMemoryInit()
{
#ifndef NO_IPC
    return m_pSharedMemManager->isInit();
#endif
    return 0;
}

void EventManager::createDefaultGroupIfNeeded()
{
    if(m_bDefaultGroupNotCreated)
    {
        EventGroup defaultGroup(addGroup(defaultGroupName));
        m_bDefaultGroupNotCreated = false;
        m_DefaultGroupId = defaultGroup.id;
    }
}
