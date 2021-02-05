//=============================================================================================================
/**
 * @file     eventgroup.cpp
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
 * @brief     EventGroup definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventgroup.h"
#include "event.h"

#include <stdlib.h>
#include <ctime>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace EVENTSINTERNAL;

//=============================================================================================================
// INIT STATIC MEMBERS
//=============================================================================================================

int EventGroup::eventGroupIdCounter(0);

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventGroup::EventGroup(const char* name)
: m_sName(name)
, m_Id(eventGroupIdCounter++)
{
    std::srand(std::time(NULL));

    setRandomColor();
}

//=============================================================================================================

EventGroup::EventGroup(const char* name,
                       const char* color)
: m_sName(name)
, m_Id(eventGroupIdCounter++)
{
    std::srand(std::time(NULL));

    setColor(color);
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

void EventGroup::setRandomColor()
{
    m_Color[0] = rand() % 256;
    m_Color[1] = rand() % 256;
    m_Color[2] = rand() % 256;
    m_Color[3] = 0xFF;
}

//=============================================================================================================

const std::string& EventGroup::getName() const
{
    return m_sName;
}

//=============================================================================================================

void EventGroup::setName(const std::string &sName)
{
    m_sName = sName;
}

//=============================================================================================================

int EventGroup::getId() const
{
    return m_Id;
}
