#include "eventmanager.h"

using namespace EVENTSLIB;


Event::Event(const int i,const  int s,const int gid)
: id(i)
, sample(s)
, groupId(gid)
{ }

Event::Event(const EVENTSINTERNAL::Event& e)
: Event(e.getId(), e.getSample(), e.getGroup())
{ }

RgbColor::RgbColor(const char rRhs, const char gRhs, const char bRhs)
: RgbColor(rRhs, gRhs, bRhs, 0xFF)
{ }

RgbColor::RgbColor(const char rRhs, const char gRhs, const char bRhs, const char aRhs)
: r(rRhs)
, g(gRhs)
, b(bRhs)
, a(aRhs)
{ }

Group::Group(int idRhs, const char* nameRhs, const RgbColor& cRhs )
: id(idRhs)
, name(nameRhs)
, color(cRhs)
{ }

Group::Group(const EVENTSINTERNAL::EventGroup& gRhs)
    : Group(gRhs.getId(), gRhs.getName(), gRhs.getColor())
{ }

//=============================================================================================================

//void EventManager::addEvent(int iSample)
//{
//    Event event(iSample, m_iSelectedGroup);
//    m_eventList.emplace(iSample, m_iSelectedGroup);
//}

////=============================================================================================================

//void EventManager::addGroup(const char *sGroupName)
//{
//    EventGroup eventGroup(sGroupName);
//    m_iSelectedGroup = eventGroup;
//    selectGroup();
//}

std::unique_ptr< std::vector<Event> > EventManager::getAllEvents()
{
    size_t numEvents(m_eventList.size());
    std::unique_ptr<std::vector<Event> > eventsList(std::make_unique<std::vector<Event> >(numEvents));

    for(const auto& e : m_eventList)
    {
        eventsList->emplace_back(e);
    }
    return eventsList;
}

std::unique_ptr<std::vector<Group> > EventManager::getAllGroups()
{
    size_t  numGroups(m_eventGroupList.size());
    std::unique_ptr<std::vector<Group> > groupsList(std::make_unique<std::vector<Event> >(numGroups));

}

using GroupIterator = std::unordered_map<int, EVENTSINTERNAL::EventGroup>::const_iterator;
