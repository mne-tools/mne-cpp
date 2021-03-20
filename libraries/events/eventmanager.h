#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include "eventsharedmemmanager.h"

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>


namespace EVENTSINTERNAL {
    class EventSharedMemManager;
}

namespace EVENTSLIB {

/**
 * The EventManager class.
 *
 * This class can be understood as an API, for the whole Event system, which is the Events library (EVENTSLIB namespace).
 */
class EVENTS_EXPORT EventManager
{
public:
    /**
     * EventManager constructor.
     */
    EventManager();

    /**
     * getNumEvents Retrive the number of events already stored in the Event system.
     * @return Number of events.
     */
    size_t getNumEvents() const;

    /**
     * getEvent Retrieve an event from its id.
     * @param[in] eventId Event id.
     * @return The event.
     */
    Event getEvent(idNum eventId) const;

    /**
     * getEvents Retrieve a set of events, given their ids.
     * @param[in] eventIds The ids of events to retrieve.
     * @return A pointer to a vector with all the events are kept.
     */
    std::unique_ptr<std::vector<Event> > getEvents(const std::vector<idNum> eventIds) const ;

    /**
     * getAllEvents Retrieve all the events in the system.
     * @return A pointer to a vector with all the events.
     */
    std::unique_ptr<std::vector<Event> > getAllEvents() const ;

    /**
     * getEventsInSample Get all the events declared in the given sample.
     * @param[in] sample The sample from where to get the events from.
     * @return A pointer to a vector whith all the events ocurring in the specified sample.
     */
    std::unique_ptr<std::vector<Event> > getEventsInSample(int sample) const ;

    /**
     * getEventsBetween Get all the events ocurring between (inclusive) two given samples.
     * @param[in] sampleStart First sample to look for events.
     * @param[in] sampleEnd Last sample to look for events.
     * @return A pointer to a vector containing all the events in between the specified samples.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const ;

    /**
     * getEventsBetween Overriden function to retrieve all the events in between (inclusive) two samples, however only
     * the ones belonging to a specified group.
     * @param[in] sampleStart First sample to look events for.
     * @param[in] sampleEnd Last sample to look for events.
     * @param[in] groupid The group to which the events have to belong.
     * @return A pointer to a vector containing all the events.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, idNum groupid) const ;

    /**
     * getEventsBetween Overriden function to retrieve all the events in between (inclusive) two samples, however only the
     * ones that belong to one of a given list of groups.
     * @param[in] sampleStart First sample to look events for.
     * @param[in] sampleEnd Last sample to look for events.
     * @param[in] groupidList The list of groups to which the events have to belong.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const std::vector<idNum>& groupIdsList) const ;

    /**
     * getEventsInGroup Retrieve all the events belonging to a specified group of events.
     * @param[in] groupId The group of events.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const idNum groupId) const;

    /**
     * getEventsInGroups Retrieve all the events belonging to a specified set of group of events.
     * @param[in] groupIdsList The set of group of events to which the events will belong to.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsInGroups(const std::vector<idNum>& groupIdsList) const;

    /**
     * addEvent Add an event at a specific sample. The event will be added to a "Default" group.
     * @param[in] sample The sample at which the event should be added.
     * @return The event created. This variable hels in case the id of the newly created event is needed.
     */
    Event addEvent(int sample);

    /**
     * addEvent Overriden function. Add event in a specific sample.
     * @param[in] sample The sample at which the event should be created.
     * @param[in] groupId The id of the event group to which the event belongs to.
     * @return the newly created event.
     */
    Event addEvent(int sample, idNum groupId);

    /**
     * moveEvent Move an event to a new sample. All other fields of the event will remain unaltered.
     * @param[in] eventId The id of the event to be moved.
     * @param[in] newSample The new sample to which the event will be moved to.
     * @return The moving operation was done succesfully.
     */
    bool moveEvent(idNum eventId, int newSample);

    /**
     * deleteEvent Delete an event.
     * @param eventId The id of the event to be deleted from the event system.
     * @return The deletion operation was successful.
     */
    bool deleteEvent(idNum eventId) noexcept;

    /**
     * deleteEvents This is an overriden function. Delete a set of events.
     * @param[in] eventIds The ids of the events to be deleted.
     * @return The deletion of all the events was successful.
     */
    bool deleteEvents(const std::vector<idNum>& eventIds);

    /**
     * deleteEvents Delete a set of events.
     * @param[in] eventIds A pointer to a vector with the events to be deleted.
     * @return All the deletion operations where successful.
     */
    bool deleteEvents(std::unique_ptr<std::vector<Event> > eventIds);

    /**
     * deleteEventsInGroup Delete all the events in a specific group.
     * @param[in] groupId The id of the group who's events are to be deleted.
     * @return All the deletion operations where sucessful.
     */
    bool deleteEventsInGroup(idNum groupId);

    /**
     * getNumGroups Retrieve the number of groups created in the event system.
     * @return An integer with the number of event groups.
     */
    int getNumGroups() const;

    /**
     * getGroup Retrieve an event group.
     * @param[in] groupId The id of the event group to be retrieved.
     * @return The event group.
     */
    EventGroup getGroup(idNum groupId) const;

    /**
     * getAllGroups Retrieve all the groups declared in the event system.
     * @return A pointer to a vector containing all the event groups existing.
     */
    std::unique_ptr<std::vector<EventGroup> > getAllGroups() const ;

    /**
     * getGroups Get group events given their ids.
     * @param[in] groupIds A list of group eveent ids to be retrieved.
     * @return A pointer to a vector storing a list of event groups.
     */
    std::unique_ptr<std::vector<EventGroup> > getGroups(const std::vector<idNum>& groupIds) const ;

    /**
     * addGroup Add a new group of events.
     * @param[in] sGroupName The group's name string.
     * @return The newly created event group.
     */
    EventGroup addGroup(const std::string& sGroupName);

    /**
     * addGroup Add a new group of events, specifying its name and color. This is an overriden funcion.
     * @param[in] sGroupName The name of the new group..
     * @param[in] color The color of the new group.
     * @return The event group created.
     */
    EventGroup addGroup(const std::string& sGroupName, const RgbColor& color);

    /**
     * deleteGroup Delete a group event.
     * @param[in] groupId The ids of the group events to be deleted.
     * @return A bool variable equals true if all the deletion operations are succesful.
     */
    bool deleteGroup(const idNum groupId);

    /**
     * deleteGroups
     * @param[in] groupIds
     * @return
     */
    bool deleteGroups(const std::vector<idNum>& groupIds);

    void renameGroup(const idNum groupId, const std::string& newName);
    void setGroupColor(const idNum groupId, const RgbColor& color);
    EventGroup mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName);
    EventGroup duplicateGroup(const idNum groupId, const std::string& newName);

    bool addEventToGroup(const idNum eventId, const idNum groupId);
    bool addEventsToGroup(const std::vector<idNum>& eventIds, const idNum groupId);

    //shared memory api. it should be that simple
    void initSharedMemory();
    void initSharedMemory(SharedMemoryMode mode);
    void stopSharedMemory();
    bool isSharedMemoryInit();

private:
    idNum generateNewEventId();
    idNum generateNewGroupId();

    void insertEvent(const EVENTSINTERNAL::EventINT& e);
    bool eraseEvent(idNum eventId);

    std::multimap<int, EVENTSINTERNAL::EventINT>::const_iterator findEventINT(idNum id) const;
    void createDefaultGroupIfNeeded();

    std::multimap<int, EVENTSINTERNAL::EventINT>    m_EventsListBySample;
    std::unordered_map<idNum, int>                  m_MapIdToSample;
    std::map<idNum, EVENTSINTERNAL::EventGroupINT>  m_GroupsList;

    std::unique_ptr<EVENTSINTERNAL::EventSharedMemManager>  m_pSharedMemManager;

    idNum                                           m_iEventIdCounter;
    idNum                                           m_iGroupIdCounter;
    bool                                            m_bDefaultGroupNotCreated;
    idNum                                           m_DefaultGroupId;

    friend class EVENTSINTERNAL::EventSharedMemManager;
};

template<typename T>
inline std::unique_ptr<std::vector<T> > allocateOutputContainer() noexcept
{
    return std::make_unique<std::vector<T> >();
};

template<typename T>
inline std::unique_ptr<std::vector<T> > allocateOutputContainer(size_t size) noexcept
{
    auto v = std::make_unique<std::vector<T> >();
    if(size > 0)
    {
        v->reserve(size);
    }
    return v;
};

}//namespace
#endif // EVENTS_H
