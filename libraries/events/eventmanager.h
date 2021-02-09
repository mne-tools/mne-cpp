#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include "eventsharedmemmanager.h"

#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

namespace EVENTSLIB {

enum SharedMemoryMode { READ, WRITE, BYDIRECTIONAL };

class EVENTS_EXPORT EventManager
{
public:
    EventManager();

    //event getters
    int getNumEvents() const;
    Event getEvent(idNum eventId) const;
    std::unique_ptr<std::vector<Event> > getEvents(const std::vector<idNum> eventIds) const noexcept;
    std::unique_ptr<std::vector<Event> > getAllEvents() const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsInSample(int sample) const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const Group& group) const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const std::vector<Group>& group) const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const idNum groupId) const noexcept;
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const Group& group) const noexcept;

    //event setters
    Event addEvent(int sample, idNum groupId);
    Event moveEvent(idNum eventId, int newSample);
    void deleteEvent(idNum eventId);
    void deleteEvents(const std::vector<idNum>& eventIds);
    void deleteEventsInGroup(idNum groupId);

    //group getters
    int getNumGroups() const;
    std::unique_ptr<std::vector<Group> > getAllGroups() const noexcept;
    Group getGroup(const idNum groupId) const;
    std::unique_ptr<std::vector<Group> > getGroups(const std::vector<idNum>& groupIds) const noexcept;

    //group setters
    Group addGroup(const std::string& sGroupName);
    Group addGroup(const std::string& sGroupName, const RgbColor& color);
    void deleteGroup(const idNum groupId);
    Group renameGroup(const idNum groupId, const std::string& newName);
    Group setGroupColor(const idNum groupId, const RgbColor& color);
    Group mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName, const RgbColor& color);
    Group duplicateGroup(const idNum groupId, const std::string& newName);

    //shared memory api. it should be that simple
    void initSharedMemory() const;
    void initSharedMemory(SharedMemoryMode mode) const;
    void stopSharedMemory() const;
    bool isSharedMemoryInit() const;

private:
    idNum generateNewEventId() const;
    idNum generateNewGroupId() const;

//    std::multiset<EVENTSINTERNAL::Event>::iterator findEventById() const;
//    std::multiset<EVENTSINTERNAL::EventGroup>::iterator findGroupById() const;

    std::multiset<EVENTSINTERNAL::Event>        m_EventList;
    std::multiset<EVENTSINTERNAL::EventGroup>   m_pGroupList;
    EVENTSINTERNAL::EventSharedMemManager       m_sharedMemManager;
    static idNum                                eventIdCounter;
    static idNum                                groupIdCounter;
};

}//namespace
#endif // EVENTS_H
