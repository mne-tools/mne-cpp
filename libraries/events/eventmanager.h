#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include "eventsharedmemmanager.h"

#include <string>
#include <set>
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
    Event getEvent(uint eventId) const;
    std::unique_ptr< std::vector<Event> > getEvents(const std::vector<uint> eventIds) const;
    std::unique_ptr< std::vector<Event> > getAllEvents() const;
    std::unique_ptr< std::vector<Event> > getEventsInSample(int sample) const;
    std::unique_ptr< std::vector<Event> > getEventsBetween(int sampleStart, int sampleEnd) const;
    std::unique_ptr< std::vector<Event> > getEventsInGroups(const std::vector<uint>& groupIds) const;
    //event setters
    void addEvent(int sample);
    void addEvents(const std::vector<int>& samples);
    void deleteEvent(uint eventId);
    void deleteEvents(const std::vector<uint>& eventIds);

    //group getters
    std::unique_ptr<std::vector<Group> > getAllGroups() const;
    Group getGroup(const uint groupId) const;
    std::unique_ptr<std::vector<Group> > getGroups(const std::vector<uint>& groupIds) const;

    //group setters
    void addGroup(const std::string& sGroupName);
    void addGroup(const std::string& sGroupName, const RgbColor& color);
    void deleteGroup(const uint groupId);
    void renameGroup(const uint groupId, const std::string& newName);
    void setGroupColor(const uint groupId, const RgbColor& color);
    void mergeGroups(const std::vector<uint>& groupIds);
    void duplicateGroup(const uint groupId, const std::string& newName);

    //shared memory api. it should be that simple
    void initSharedMemory() const;
    void initSharedMemory(SharedMemoryMode mode) const;
    void stopSharedMemory() const;
    bool isSharedMemoryInit() const;

private:
    std::set<EVENTSINTERNAL::Event>                         m_eventList;
    std::unordered_map<int, EVENTSINTERNAL::EventGroup>     m_eventGroupList;
    std::shared_ptr<EVENTSINTERNAL::EventSharedMemManager>  m_pSharedMemManager;
};

}//namespace
#endif // EVENTS_H
