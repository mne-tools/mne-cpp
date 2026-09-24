//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     eventgroup.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>
 * @since    0.1.8
 * @date     February, 2021
 * @brief     EventGroup declaration.
 */

#ifndef EVENTGROUP_EVENTS_H
#define EVENTGROUP_EVENTS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <string>
#include <cstdint>
#include "events_global.h"

//=============================================================================================================
// LOCAL DEFINITIONS
//=============================================================================================================

using idNum = unsigned int; //We want to eventually modify the idNum to be a hash string or a user-defined class. For now, just an int.
using uchar = unsigned char;

//=============================================================================================================
// NAMESPACE EVENTSLIB
//=============================================================================================================

namespace EVENTSLIB {

//=============================================================================================================
// EVENTSINTERNAL FORWARD DECLARATIONS
//=============================================================================================================

namespace EVENTSINTERNAL {
    class EventGroupINT;
}

//=============================================================================================================
/**
 * @brief Simple RGB triplet for color-coding event groups
 *
 * A holder for the color of an event group.
 *
 * This class interacts well with QtColor, since it has constructors dependant on rgb type data.
 */
struct EVENTS_EXPORT RgbColor
{
    //=========================================================================================================
    /**
     * Contructs an RgbColor object. The color will be set with the defaultGroupColor variable.
     */
    RgbColor();

    //=========================================================================================================
    /**
     * Conttructs an RgbColor object with the specified RGB values.
     *
     * @param [in] rRhs R value
     * @param [in] gRhs G value
     * @param [in] bRhs B value
     */
    RgbColor(const uchar rRhs, const uchar gRhs, const uchar bRhs);

    //=========================================================================================================
    /**
     * Constructs an RgbColor object with the specified RGB and transparency values.
     *
     * @param [in] rRhs R value
     * @param [in] gRhs G value
     * @param [in] bRhs B value
     * @param [in] aRhs Transparency value
     */
    RgbColor(const uchar rRhs, const uchar gRhs,
             const uchar bRhs, const uchar aRhs);

    uchar r;    /**< Red value. */
    uchar g;    /**< Green value. */
    uchar b;    /**< Blue value. */
    uchar a;    /**< Transparency value. */
};

/**
 * EventGroup Class
 *
 * @brief EventGroup class is designed as a data holder for a group. It is designed towards ease of use for a client of
 * the Events library. It's counterpart EventGroupINT is intended to be used internally by the Event library classes.
 */
struct EVENTS_EXPORT EventGroup
{

    //=========================================================================================================
    /**
    * Constructs an EventGroup external sctruct.
    */
    EventGroup() = default;

    //=========================================================================================================
    /**
     * EventGroup copy constructor.
     *
     * @param [in] g rhs EventGroup
     */
    EventGroup(const EVENTSLIB::EventGroup& g);

    //=========================================================================================================
    /**
     * EventGroup constructor based on an EventGroupINT.
     *
     * @param [in] g EventGroupInt input.
     */
    EventGroup(const EVENTSINTERNAL::EventGroupINT& g);

    idNum           id;     /**< Id of the event group. */
    std::string     name;   /**< Name of the event group. */
    RgbColor        color;  /**< Color of the event. */
    int             order;  /**< Holder for an ordering variable. */
};

namespace EVENTSINTERNAL {

/**
 * EventGroupINT class.
 *
 * @brief the class stores the concept of an event group internally in the Event library.
 */
class EventGroupINT
{
public:

    //=========================================================================================================
    /**
     * EventGroupINT constructor.
     *
     * @param [in] name name of the group.
     */
    EventGroupINT(const char* name);

    //=========================================================================================================
    /**
     * EventGroupINT constructorbased on an rvalue std::string.
     *
     * @param name name of the created group.
     */
    EventGroupINT(std::string&& name);

    //=========================================================================================================
    /**
     * EventGroupINT constructor based on a groupId and a name.
     *
     * @param [in] id   Id of the new group.
     * @param [in] name Name of the new group.
     */
    EventGroupINT(idNum id, const std::string& name);

    //=========================================================================================================
    /**
     * EventGroupINT constructor.
     *
     * @param [in] id   Id of the new group.
     * @param [in] name Name of the new group.
     * @param [in] color  Color of the new group.
     */
    EventGroupINT(idNum id, const std::string& name, const EVENTSLIB::RgbColor& color);

    //=========================================================================================================
    /**
     * setColor Set the color of the group.
     *
     * @param [in] color New color of the group.
     */
    void setColor(const EVENTSLIB::RgbColor& color);

    //=========================================================================================================
    /**
     * setRandomColor Make the group adopt a random color.
     */
    void setRandomColor();

    //=========================================================================================================
    /**
     * getColor Get the group color.
     *
     * @return Color of the group.
     */
    EVENTSLIB::RgbColor getColor() const;

    //=========================================================================================================
    /**
     * getName Retrieve the name of the group.
     *
     * @return Group name
     */
    const std::string& getName() const;

    //=========================================================================================================
    /**
     * setName Set the name of the group.
     *
     * @param sName New name of the group.
     */
    void setName(const std::string& sName);

    //=========================================================================================================
    /**
     * getId Retrieve the group id.
     *
     * @return Id of the group
     */
    idNum getId() const;

    //=========================================================================================================
    /**
     * getDescription Get the group destriction.
     *
     * @return description
     */
    std::string getDescription() const;

    //=========================================================================================================
    /**
     * getOrder Get the order of the group.
     *
     * @return order
     */
    int getOrder() const;

    //=========================================================================================================
    /**
     * setOrder Set the group order.
     *
     * @param order
     */
    void setOrder(int order);

    //=========================================================================================================
    /**
     * operator < Overriden operator <.
     *
     * @param groupRHS object to which compare.
     *
     * @return results of comparison
     */
    bool operator<(const EventGroupINT& groupRHS) const;

private:
    std::string         m_sName;            /**< Group name. */
    EVENTSLIB::RgbColor m_Color;            /**< Group color. */
    idNum               m_Id;               /**< Group Id.*/
    std::string         m_sDescription;     /**< Group description text.*/
    int                 m_order;            /**< Group order placeholder.*/
};

} //namespace EVENTSINTERNAL
} //namespace EVENTSLIB
#endif // EVENTGROUP_H
