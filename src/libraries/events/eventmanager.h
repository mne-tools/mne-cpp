//=============================================================================================================
/**
 * @file     eventmanager.h
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
 * @brief     EventManager declaration.
 *
 */

#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"

#ifndef NO_IPC
#include "eventsharedmemmanager.h"
#else
namespace EVENTSLIB{
    enum SharedMemoryMode { READ, WRITE, READWRITE };
}
#endif
//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

//=============================================================================================================
// NAMESPACE EVENTSLIB
//=============================================================================================================

namespace EVENTSLIB {

//=============================================================================================================
// EVENTSINTERNAL FORWARD DECLARATIONS
//=============================================================================================================

namespace EVENTSINTERNAL {
    class EventSharedMemManager;
}

//=============================================================================================================
/**
 * The EventManager class.
 *
 * This class can be understood as an API, for the whole Event system, which is the Events library (EVENTSLIB
 * namespace).
 */
class EVENTS_EXPORT EventManager
{
public:
    //=========================================================================================================
    /**
     * EventManager constructor.
     */
    EventManager();

    //=========================================================================================================
    /**
     * Retrive the number of events already stored in the Event system.
     * @return Number of events.
     */
    size_t getNumEvents() const;

    //=========================================================================================================
    /**
     * Retrieve an event from its id.
     * @param[in] eventId Event id.
     * @return The event.
     */
    Event getEvent(idNum eventId) const;

    //=========================================================================================================
    /**
     * Retrieve a set of events, given their ids.
     * @param[in] eventIds The ids of events to retrieve.
     * @return A pointer to a vector with all the events are kept.
     */
    std::unique_ptr<std::vector<Event> > getEvents(const std::vector<idNum> eventIds) const ;

    //=========================================================================================================
    /**
     * Retrieve all the events in the system.
     * @return A pointer to a vector with all the events.
     */
    std::unique_ptr<std::vector<Event> > getAllEvents() const ;

    //=========================================================================================================
    /**
     * Get all the events declared in the given sample.
     * @param[in] sample The sample from where to get the events from.
     * @return A pointer to a vector whith all the events ocurring in the specified sample.
     */
    std::unique_ptr<std::vector<Event> > getEventsInSample(int sample) const ;

    //=========================================================================================================
    /**
     * getEventsBetween Get all the events ocurring between (inclusive) two given samples.
     * @param[in] sampleStart First sample to look for events.
     * @param[in] sampleEnd Last sample to look for events.
     * @return A pointer to a vector containing all the events in between the specified samples.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const ;

    //=========================================================================================================
    /**
     * Overriden function to retrieve all the events in between (inclusive) two samples, however only
     * the ones belonging to a specified group.
     * @param[in] sampleStart First sample to look events for.
     * @param[in] sampleEnd Last sample to look for events.
     * @param[in] groupid The group to which the events have to belong.
     * @return A pointer to a vector containing all the events.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, idNum groupid) const ;

    //=========================================================================================================
    /**
     * Overriden function to retrieve all the events in between (inclusive) two samples, however only the
     * ones that belong to one of a given list of groups.
     * @param[in] sampleStart First sample to look events for.
     * @param[in] sampleEnd Last sample to look for events.
     * @param[in] groupidList The list of groups to which the events have to belong.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const std::vector<idNum>& groupIdsList) const ;

    //=========================================================================================================
    /**
     * Retrieve all the events belonging to a specified group of events.
     * @param[in] groupId The group of events.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const idNum groupId) const;

    //=========================================================================================================
    /**
     * Retrieve all the events belonging to a specified set of group of events.
     * @param[in] groupIdsList The set of group of events to which the events will belong to.
     * @return A pointer to a vector containing all the events found.
     */
    std::unique_ptr<std::vector<Event> > getEventsInGroups(const std::vector<idNum>& groupIdsList) const;

    //=========================================================================================================
    /**
     * Add an event at a specific sample. The event will be added to a "Default" group.
     * @param[in] sample The sample at which the event should be added.
     * @return The event created. This variable hels in case the id of the newly created event is needed.
     */
    Event addEvent(int sample);

    //=========================================================================================================
    /**
     * Overriden function. Add event in a specific sample.
     * @param[in] sample The sample at which the event should be created.
     * @param[in] groupId The id of the event group to which the event belongs to.
     * @return the newly created event.
     */
    Event addEvent(int sample, idNum groupId);

    //=========================================================================================================
    /**
     * Move an event to a new sample. All other fields of the event will remain unaltered.
     * @param[in] eventId The id of the event to be moved.
     * @param[in] newSample The new sample to which the event will be moved to.
     * @return The moving operation was done succesfully.
     */
    bool moveEvent(idNum eventId, int newSample);

    //=========================================================================================================
    /**
     * Delete an event.
     * @param eventId The id of the event to be deleted from the event system.
     * @return The deletion operation was successful.
     */
    bool deleteEvent(idNum eventId) noexcept;

    //=========================================================================================================
    /**
     * This is an overriden function. Delete a set of events.
     * @param[in] eventIds The ids of the events to be deleted.
     * @return The deletion of all the events was successful.
     */
    bool deleteEvents(const std::vector<idNum>& eventIds);

    //=========================================================================================================
    /**
     * deleteEvents Delete a set of events.
     * @param[in] eventIds A pointer to a vector with the events to be deleted.
     * @return All the deletion operations where successful.
     */
    bool deleteEvents(std::unique_ptr<std::vector<Event> > eventIds);

    //=========================================================================================================
    /**
     * Delete all the events in a specific group.
     * @param[in] groupId The id of the group who's events are to be deleted.
     * @return All the deletion operations where sucessful.
     */
    bool deleteEventsInGroup(idNum groupId);

    //=========================================================================================================
    /**
     * Retrieve the number of groups created in the event system.
     * @return An integer with the number of event groups.
     */
    int getNumGroups() const;

    //=========================================================================================================
    /**
     * Retrieve an event group.
     * @param[in] groupId The id of the event group to be retrieved.
     * @return The event group.
     */
    EventGroup getGroup(idNum groupId) const;

    //=========================================================================================================
    /**
     * Retrieve all the groups declared in the event system.
     * @return A pointer to a vector containing all the event groups existing.
     */
    std::unique_ptr<std::vector<EventGroup> > getAllGroups() const ;

    //=========================================================================================================
    /**
     * Get group events given their ids.
     * @param[in] groupIds A list of group eveent ids to be retrieved.
     * @return A pointer to a vector storing a list of event groups.
     */
    std::unique_ptr<std::vector<EventGroup> > getGroups(const std::vector<idNum>& groupIds) const ;

    //=========================================================================================================
    /**
     * Add a new group of events.
     * @param[in] sGroupName The group's name string.
     * @return The newly created event group.
     */
    EventGroup addGroup(const std::string& sGroupName);

    //=========================================================================================================
    /**
     * Add a new group of events, specifying its name and color. This is an overriden funcion.
     * @param[in] sGroupName The name of the new group..
     * @param[in] color The color of the new group.
     * @return The event group created.
     */
    EventGroup addGroup(const std::string& sGroupName, const RgbColor& color);

    //=========================================================================================================
    /**
     * Delete a group. If there are events in the group the group won't be deleted.
     * @param[in] groupId The ids of the group events to be deleted.
     * @return A bool variable equals true if all the deletion operations are succesful.
     */
    bool deleteGroup(const idNum groupId);

    //=========================================================================================================
    /**
     * Delete all groups. If any of the groups has events in it, that group will not be deleted.
     * @param[in] groupIds
     * @return
     */
    bool deleteGroups(const std::vector<idNum>& groupIds);

    //=========================================================================================================
    /**
     * Change the name of the specified group (groupId).
     * @param groupId Id of the group.
     * @param newName New string to use as the group name.
     */
    void renameGroup(const idNum groupId, const std::string& newName);

    //=========================================================================================================
    /**
     * Change the color of the group to a specified one.
     * @param groupId Group identifier.
     * @param color New color for the group.
     */
    void setGroupColor(const idNum groupId, const RgbColor& color);

    //=========================================================================================================
    /**
     * Merge two groups together into a diferent third group.
     * @param groupIds Group Ids of the groups to merge.
     * @param newName New name to be applied.
     * @return The actual new group created from the merge.
     */
    EventGroup mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName);

    //=========================================================================================================
    /**
     * Duplicate a specified group. All the events in the group will also  be duplicated.
     * @param groupId
     * @param newName
     * @return The actual new group created.
     */
    EventGroup duplicateGroup(const idNum groupId, const std::string& newName);

    //=========================================================================================================
    /**
     * Add one event to a group.
     * @param eventId Identifier for the event.
     * @param groupId Group to add the event to.
     * @return Boolean stating if the operation was successful.
     */
    bool addEventToGroup(const idNum eventId, const idNum groupId);

    //=========================================================================================================
    /**
     * Add more than one event to a group.
     * @param eventIds The identifiers for the events to add.
     * @param groupId The group to add the events to.
     * @return Boolean variable stating the success of the operation.
     */
    bool addEventsToGroup(const std::vector<idNum>& eventIds, const idNum groupId);

    //=========================================================================================================
    /**
     * Initialize the shared memory mecanism.
     * This mecanism allows for inter-process communication. Typically this would be with mne-scan.
     */
    void initSharedMemory();

    //=========================================================================================================
    /**
     * Initialize the shared memory space with an empty linked list.
     * @param mode Enum stating the mode to use for in the shared memory.
     */
    void initSharedMemory(SharedMemoryMode mode);

    //=========================================================================================================
    /**
     * Disable the shared memory management.
     */
    void stopSharedMemory();

    //=========================================================================================================
    /**
     * Getter to know if the shared memory mechanism has been initialized already.
     * @return Bool value stating if the shared memoy mechanism.
     */
    bool isSharedMemoryInit();

private:
    //=========================================================================================================
    /**
     * Automatically generate a new group id so that the class can assign iim to a new event.
     * @return
     */
    idNum generateNewEventId();

    //=========================================================================================================
    /**
     * Automatically generate a new group id so that the class can asign it to a new group.
     * @return
     */
    idNum generateNewGroupId();

    //=========================================================================================================
    /**
     * Insert an event in the internal storage list of events.
     * @param e The event to be inserted.
     */
    void insertEvent(const EVENTSINTERNAL::EventINT& e);

    //=========================================================================================================
    /**
     * Delete an event from the system.
     * @param eventId The id of the event to erase.
     * @return
     */
    bool eraseEvent(idNum eventId);

    //=========================================================================================================
    /**
     * Find and event in the system, given it's id.
     * @param id The id of the event to find.
     * @return
     */
    std::multimap<int, EVENTSINTERNAL::EventINT>::const_iterator findEventINT(idNum id) const;

    //=========================================================================================================
    /**
     * Check if the default group has been created already and it has never been deleted.
     */
    void createDefaultGroupIfNeeded();

    std::multimap<int, EVENTSINTERNAL::EventINT>    m_EventsListBySample;           /**< List of events organized by sample.*/
    std::unordered_map<idNum, int>                  m_MapIdToSample;                /**< EventId to sample relationship table.*/
    std::map<idNum, EVENTSINTERNAL::EventGroupINT>  m_GroupsList;                   /**< Storage of eventgroups.*/

#ifndef NO_IPC
    std::unique_ptr<EVENTSINTERNAL::EventSharedMemManager>  m_pSharedMemManager;    /**< Pointer to a shared manager object.*/
    friend class EVENTSINTERNAL::EventSharedMemManager;
#endif

    idNum   m_iEventIdCounter;          /**< This counter will serve as an eventId until this get error-prone. However it can be easily updated.*/
    idNum   m_iGroupIdCounter;          /**< This counter will serve as a groupId counter, and id generator.*/
    bool    m_bDefaultGroupNotCreated;  /**< State variable to know if the default group has been created, and thus decide to create it again or not. */
    idNum   m_DefaultGroupId;           /**< The id of the default group.*/

};

//=============================================================================================================
/**
 * Prepare memory allocation for the output of one of the methods in the class.
 */
template<typename T>
inline std::unique_ptr<std::vector<T> > allocateOutputContainer() noexcept
{
    return std::make_unique<std::vector<T> >();
};

//=============================================================================================================
/**
 * Prepare memory allocation for the output of one of the methods in the class.
 */
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

}//namespace EVENTSLIB
#endif // EVENTS_H
