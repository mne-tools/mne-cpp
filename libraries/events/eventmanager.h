#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include "eventsharedmemmanager.h"

#include <string>
#include <optional>
#include <map>
#include <unordered_map>
#include <vector>
#include <memory>


namespace EVENTSINTERNAL {
    class EventSharedMemManager;
}

namespace EVENTSLIB {

class EVENTS_EXPORT EventManager
{
public:
    EventManager();

    //event getters
    size_t getNumEvents() const;
    std::optional<Event> getEvent(idNum eventId) const;
    std::unique_ptr<std::vector<Event> > getEvents(const std::vector<idNum> eventIds) const ;
    std::unique_ptr<std::vector<Event> > getAllEvents() const ;
    std::unique_ptr<std::vector<Event> > getEventsInSample(int sample) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, idNum groupid) const ;
    std::unique_ptr<std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd, const std::vector<idNum>& groupIdsList) const ;
    std::unique_ptr<std::vector<Event> > getEventsInGroup(const idNum groupId) const;

    //event setters
    Event addEvent(int sample, idNum groupId);
    bool moveEvent(idNum eventId, int newSample);
    bool deleteEvent(idNum eventId) noexcept;
    bool deleteEvents(const std::vector<idNum>& eventIds);
    bool deleteEvents(std::unique_ptr<std::vector<Event> > eventIds);
    bool deleteEventsInGroup(idNum groupId);

    //group getters
    int getNumGroups() const;
    std::optional<EventGroup> getGroup(idNum groupId) const;
    std::unique_ptr<std::vector<EventGroup> > getAllGroups() const ;
    std::unique_ptr<std::vector<EventGroup> > getGroups(const std::vector<idNum>& groupIds) const ;

    //group setters
    EventGroup addGroup(const std::string& sGroupName);
    EventGroup addGroup(const std::string& sGroupName, const RgbColor& color);
    bool deleteGroup(const idNum groupId);
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
    idNum generateNewEventId() const;
    idNum generateNewGroupId() const;

    void insertEvent(const EVENTSINTERNAL::EventINT& e);
    std::optional<std::multimap<const int, EVENTSINTERNAL::EventINT>::const_iterator>
    findEventINT(idNum id) const;

    std::multimap<int, EVENTSINTERNAL::EventINT>    m_EventsListBySample;
    std::unordered_map<idNum, int>                  m_MapIdToSample;
    std::map<idNum, EVENTSINTERNAL::EventGroupINT>  m_GroupsList;

    std::unique_ptr<EVENTSINTERNAL::EventSharedMemManager>  m_pSharedMemManager;

    static idNum                                    m_iEventIdCounter;
    static idNum                                    m_iGroupIdCounter;
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
