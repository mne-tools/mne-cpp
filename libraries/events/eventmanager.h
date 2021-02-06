#ifndef EVENTMANAGER_EVENTS_H
#define EVENTMANAGER_EVENTS_H

#include "events_global.h"
#include "event.h"
#include "eventgroup.h"
#include <set>
#include <unordered_map>
#include <vector>
#include <memory>

namespace EVENTSLIB {

struct EVENTS_EXPORT Event
{
   Event(const int, const int, const int);
   Event(const EVENTSINTERNAL::Event&);

   int  id;
   int  sample;
   int  groupId;
};

struct EVENTS_EXPORT RgbColor
{
    RgbColor(const char rRhs, const char gRhs, const char bRhs);
    RgbColor(const char rRhs, const char gRhs, const char bRhs, const char aRhs);
    char r;
    char g;
    char b;
    char a;
};

struct EVENTS_EXPORT Group
{
    Group(int idRhs, const char* nameRhs, const RgbColor& cRhs );
    Group(const EVENTSINTERNAL::EventGroup& gRhs);
    int         id;
    std::string name;
    RgbColor    color;
};

class EVENTS_EXPORT EventManager
{
public:
    EventManager();

    std::unique_ptr< std::vector<Event> > getAllEvents();

    std::unique_ptr<std::vector<Group> > getAllGroups();

    void addEvent(int iSample);

    void addGroup(const char* sGroupName);

private:
    std::set<EVENTSINTERNAL::Event>             m_eventList;
    std::unordered_map<int, EVENTSINTERNAL::EventGroup>   m_eventGroupList;
};

}//namespace
#endif // EVENTS_H
