//=============================================================================================================
/**
 * @file     event.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>;
 *           Juan Garcia-Prieto <juangpc@gmail.com>
 * @since    0.1.8
 * @date     January, 2021
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
 * @brief     Event and EventList definitions.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "event.h"
#include <stdlib.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace RTPROCESSINGLIB;

//=============================================================================================================
// DEFINE STATIC MEMBER METHODS
//=============================================================================================================

QList<Event> EventList::m_lEvents;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


EventGroupList eventGroups;

eventGroups.list.push_back()

Event::Event(int iSampleStart)
: Event(iSampleStart, defaultGroup, 0)
{

}

//=============================================================================================================

Event::Event(int iSampleStart, const EventGroup& group)
: Event(iSampleStart, group, 0)
{

}

//=============================================================================================================

Event::Event(int iSample, const EventGroup& group, int iType)
: m_iSample(iSample)
, m_iGroup(group)
, m_iType(iType)
{

}

//=============================================================================================================

Event::Event(const Event &event)
: Event(event.m_iSample, event.m_iType, event.m_iGroup)
{

}

//=============================================================================================================

int Event::getSample() const
{
    return m_iSample;
}

//=============================================================================================================

int Event::getType() const
{
    return m_iType;
}

//=============================================================================================================

int Event::getGroup() const
{
    return m_iGroup;
}

//=============================================================================================================

bool Event::operator<(const Event& rhs) const
{
   return m_iSample < rhs.getSample();
}

//=============================================================================================================

EventList::EventList()
{

}

//=============================================================================================================

void EventList::addEvent(const Event& event)
{

}

//=============================================================================================================

void EventList::addEvent(const Event& event)
{
    m_lEvents.append(event);
}

//=============================================================================================================

int EventList::getNumberOfEvents() const
{
    return m_lEvents.size();
}

//=============================================================================================================

Event EventList::getEvent(int iIndex) const
{
    return m_lEvents.at(iIndex);
}

//=============================================================================================================

void EventList::clear()
{
    m_lEvents.clear();
}

//=============================================================================================================

bool EventList::isEmpty() const
{
    return m_lEvents.isEmpty();
}

//=============================================================================================================

void EventList::insert(int iIndex,
                       const Event &event)
{
    m_lEvents.insert(iIndex, event);
}

//=============================================================================================================

void EventList::remove(int iIndex,
                       int iSpan)
{
    for(int i = 0; i < iSpan; i++){
        m_lEvents.removeAt(iIndex);
    }
}

//=============================================================================================================

Event& EventList::operator[](int i)
{
    return m_lEvents[i];
}

//=============================================================================================================

const Event& EventList::operator[](int i) const
{
    return m_lEvents.at(i);
}


//=============================================================================================================

EventGroup::EventGroup(const char* name)
: m_sName(name)
{
    setRandomColor();
    setRandomId();

}

//=============================================================================================================

void EventGroup::setColor(const char* color)
{
    m_Color[0] = color[0];
    m_Color[1] = color[1];
    m_Color[2] = color[2];
    m_Color[3] = color[3];
}


//=============================================================================================================

void EventGroup::setRandomId()
{
    m_Id = rand();
}

//=============================================================================================================

void EventGroup::setRandomColor()
{
    m_Color[0] = rand() % 256;
    m_Color[1] = rand() % 256;
    m_Color[2] = rand() % 256;
    m_Color[3] = 0xFF;
}

std::string EventGroup::getName() const
{
    return m_sName;
}
