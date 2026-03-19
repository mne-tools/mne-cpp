//=============================================================================================================
/**
 * @file     event.cpp
 * @author   Juan Garcia-Prieto <juangpc@gmail.com>
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
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
 * @brief     Event definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "event.h"
#include "eventgroup.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EVENTSLIB;

//=============================================================================================================
// INIT STATIC MEMBERS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Event::Event()
:Event(0,0,0)
{ }

//=============================================================================================================

Event::Event(const idNum id,const  int sample, const idNum groupId)
: id(id)
, sample(sample)
, groupId(groupId)
{ }

//=============================================================================================================

Event::Event(const EVENTSINTERNAL::EventINT& e)
: Event(e.getId(), e.getSample(), e.getGroupId())
{ }

//=============================================================================================================

EVENTSINTERNAL::EventINT::EventINT(idNum id)
: EventINT(id, 0, 0)
{ }

//=============================================================================================================

EVENTSINTERNAL::EventINT::EventINT(idNum id, int iSample, idNum groupId)
: m_iId(id)
, m_iSample(iSample)
, m_iGroup(groupId)
, m_sDescription("")
{ }

//=============================================================================================================

EVENTSINTERNAL::EventINT::EventINT(const EventINT& rhs)
: m_iId(rhs.getId())
, m_iSample(rhs.getSample())
, m_iGroup(rhs.getGroupId())
, m_sDescription(rhs.getDescription())
{ }

//=============================================================================================================

EVENTSINTERNAL::EventINT::EventINT(EventINT&& other)
: m_iId(other.getId())
, m_iSample(other.getSample())
, m_iGroup(other.getGroupId())
, m_sDescription(other.getDescription())
{ }

//=============================================================================================================

int EVENTSINTERNAL::EventINT::getSample() const
{
    return m_iSample;
}

//=============================================================================================================

void EVENTSINTERNAL::EventINT::setSample(int iSample)
{
    m_iSample = iSample;
}

//=============================================================================================================

idNum EVENTSINTERNAL::EventINT::getGroupId() const
{
    return m_iGroup;
}

//=============================================================================================================

void EVENTSINTERNAL::EventINT::setGroupId(idNum iGroup)
{
    m_iGroup = iGroup;
}

//=============================================================================================================

idNum EVENTSINTERNAL::EventINT::getId() const
{
    return m_iId;
}

//=============================================================================================================

std::string EVENTSINTERNAL::EventINT::getDescription() const
{
    return m_sDescription;
}

//=============================================================================================================

bool EVENTSINTERNAL::EventINT::operator<(const EventINT& rhs) const
{
    return m_iSample < rhs.getSample();
}

//=============================================================================================================

bool EVENTSINTERNAL::EventINT::operator==(const EventINT& rhs) const
{
    return (m_iId == rhs.getId());
}

EVENTSINTERNAL::EventINT EVENTSINTERNAL::EventINT::operator=(const EventINT& rhs)
{
    return EVENTSINTERNAL::EventINT(rhs);
}
