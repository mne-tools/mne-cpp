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

namespace EVENTSLIB {

enum SharedMemoryMode { READ, WRITE, BYDIRECTIONAL };

class EVENTS_EXPORT EventManager
{
public:
    EventManager();

    //event getters
    int getNumEvents() const;
    Event getEvent(idNum eventId) const;
    std::unique_ptr<std::vector<Event> > getEvents(const std::vector<idNum> eventIds) const ;
    std::unique_ptr<std::vector<Event> > getAllEvents() const ;
    std::unique_ptr<std::vector<Event> > getEventsInSample(int sample) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, idNum groupid) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const std::vector<idNum>& groupIdsList) const ;
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const idNum groupId) const ;

    //event setters
    Event addEvent(int sample, idNum groupId);
    Event moveEvent(idNum eventId, int newSample);
    void deleteEvent(idNum eventId) noexcept;
    void deleteEvents(const std::vector<idNum>& eventIds);
    void deleteEventsInGroup(idNum groupId);

    //group getters
    int getNumGroups() const;
    EventGroup getGroup(idNum groupId) const;
    std::unique_ptr<std::vector<EventGroup> > getAllGroups() const ;
    std::unique_ptr<std::vector<EventGroup> > getGroups(const std::vector<idNum>& groupIds) const ;

    //group setters
    EventGroup addGroup(const std::string& sGroupName);
    EventGroup addGroup(const std::string& sGroupName, const RgbColor& color);
    void deleteGroup(const idNum groupId);
    void deleteGroups(const std::vector<idNum>& groupIds);

    EventGroup renameGroup(const idNum groupId, const std::string& newName);
    EventGroup setGroupColor(const idNum groupId, const RgbColor& color);
    EventGroup mergeGroups(const std::vector<idNum>& groupIds, const std::string& newName);
    EventGroup duplicateGroup(const idNum groupId, const std::string& newName);

    void addEventToGroup(const idNum eventId, const idNum groupId);
    void addEventsToGroup(const std::vector<idNum>& eventIds, const idNum groupId);

    //shared memory api. it should be that simple
    void initSharedMemory() const;
    void initSharedMemory(SharedMemoryMode mode) const;
    void stopSharedMemory() const;
    bool isSharedMemoryInit() const;

private:
    idNum generateNewEventId() const;
    idNum generateNewGroupId() const;

    void insertEvent(const EVENTSINTERNAL::EventINT& e);
    auto findEventINT(idNum id) const;

    std::multimap<int, EVENTSINTERNAL::EventINT>    m_EventsListBySample;
    std::unordered_map<idNum, int>                  m_MapIdToSample;
    std::map<idNum, EVENTSINTERNAL::EventGroupINT>  m_GroupsList;

    EVENTSINTERNAL::EventSharedMemManager           m_sharedMemManager;

    static idNum                                    eventIdCounter;
    static idNum                                    groupIdCounter;
};

template<typename T>
inline std::unique_ptr<std::vector<T> > allocateOutputContainer() noexcept
{
    return std::make_unique<std::vector<T> >();
};

template<typename T>
inline std::unique_ptr<std::vector<T> > allocateOutputContainer(int size) noexcept
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
