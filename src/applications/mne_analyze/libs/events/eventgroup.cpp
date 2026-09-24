//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     eventgroup.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.8
 * @date     February, 2021
 * @brief     EventGroup definition.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventgroup.h"
#include "event.h"

#include <random>

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

constexpr static const unsigned char defaultGroupColor[] = { 0xC0, 0xFF, 0xEE };    /**< Default GroupColor. */
constexpr static const unsigned char defaultGroupTransparency = 0xFF;               /**< Default GroupTransparency value. */

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

RgbColor::RgbColor()
: RgbColor(defaultGroupColor[0], defaultGroupColor[1], defaultGroupColor[2])
{ };

//=============================================================================================================

RgbColor::RgbColor(const uchar rRhs, const uchar gRhs, const uchar bRhs)
: RgbColor(rRhs, gRhs, bRhs, defaultGroupTransparency)
{ };

//=============================================================================================================

RgbColor::RgbColor(const uchar rRhs, const uchar gRhs,
                              const uchar bRhs, const uchar aRhs)
: r(rRhs)
, g(gRhs)
, b(bRhs)
, a(aRhs)
{ };

//=============================================================================================================

EventGroup::EventGroup(const EventGroup& g)
: id(g.id)
, name(g.name)
, color(g.color)
, order(g.order)
{

}

//=============================================================================================================

EventGroup::EventGroup(const EVENTSINTERNAL::EventGroupINT& g)
: id(g.getId())
, name(g.getName())
, color(g.getColor())
, order(g.getOrder())
{

}

//=============================================================================================================

EVENTSINTERNAL::EventGroupINT::EventGroupINT(const char* name)
: EventGroupINT(std::string(name))
{

}

//=============================================================================================================

EVENTSINTERNAL::EventGroupINT::EventGroupINT(std::string&& name)
: m_sName(std::move(name))
, m_Id(0)
, m_order(0)
{
    setRandomColor();
}

//=============================================================================================================

EVENTSINTERNAL::EventGroupINT::EventGroupINT(idNum id, const std::string& name)
: m_sName(name)
, m_Id(id)
, m_order(0)
{
    setRandomColor();
}

//=============================================================================================================

EVENTSINTERNAL::EventGroupINT::EventGroupINT(idNum id, const std::string& name,
                       const RgbColor& color)
: m_sName(name)
, m_Id(id)
, m_order(0)
{
    setColor(color);
}

//=============================================================================================================

void EVENTSINTERNAL::EventGroupINT::setColor(const RgbColor& color)
{
    m_Color = color;
}

//=============================================================================================================

RgbColor EVENTSINTERNAL::EventGroupINT::getColor() const
{
    return m_Color;
}

//=============================================================================================================

void EVENTSINTERNAL::EventGroupINT::setRandomColor()
{
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, 255);
    m_Color.r = dist(rng);
    m_Color.g = dist(rng);
    m_Color.b = dist(rng);
}

//=============================================================================================================

const std::string& EVENTSINTERNAL::EventGroupINT::getName() const
{
    return m_sName;
}

//=============================================================================================================

void EVENTSINTERNAL::EventGroupINT::setName(const std::string &sName)
{
    m_sName = sName;
}

//=============================================================================================================

idNum EVENTSINTERNAL::EventGroupINT::getId() const
{
    return m_Id;
}

//=============================================================================================================

std::string EVENTSINTERNAL::EventGroupINT::getDescription() const
{
    return m_sDescription;
}

//=============================================================================================================

int EVENTSINTERNAL::EventGroupINT::getOrder() const
{
    return m_order;
}

//=============================================================================================================

void EVENTSINTERNAL::EventGroupINT::setOrder(int order)
{
    m_order = order;
}

//=============================================================================================================

bool EVENTSINTERNAL::EventGroupINT::operator<(const EventGroupINT &groupRHS) const
{
    return m_Id < groupRHS.getId();
}

