#ifndef EVENT_EVENTSINTERNAL_H
#define EVENT_EVENTSINTERNAL_H

#include "events_global.h"
#include "eventgroup.h"
#include <string>

namespace EVENTSINTERNAL {
class EventINT;
}

namespace EVENTSLIB {
/**
 * @brief This is a public Class to organize events and make it easy to manipulate
 * for the end-user of the library.
 */
struct EVENTS_EXPORT Event
{
    /**
     * @brief Event
     */
    Event();
    //=========================================================================================================
    /**
     * @brief Event
     * @param idRHS
     * @param sampleRHS
     * @param groupIdRHS
     */
    Event(const idNum idRHS,const  int sampleRHS, const idNum groupIdRHS);

    //=========================================================================================================
    /**
     * @brief Event
     * @param e
     */
    Event(const EVENTSINTERNAL::EventINT& e);

    idNum  id;
    idNum  groupId;
    int  sample;
};

}

namespace EVENTSINTERNAL {

class EventGroupINT;

// The fact that we go with int for sample is a fundamental limitation of this
// whole architecture. With a Fs = 1kHz, we could have a maximum of aprox. 25 days.
// Yes not a big limitation... if we keep using 1kHz...
// If we were to go for long longs for sample... with that same 1kHz, we could go
// recording, single file... for about 300 million years. That's that...
// at some point we can substitute int and idNum with std::int64_t. That will take that limitation away.
// I don't see how I should ever think of this again.

class EventINT
{
public:
    //=========================================================================================================
    /**
     * Create an event at sample iSample
     *
     * @param iSample
     * @param group
     */
    EventINT(idNum id);
    EventINT(idNum id, int iSample, idNum groupId);
    EventINT(const EventINT& rhs);
    EventINT(EventINT&& other);

    static inline EventINT fromSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event sample
     *
     * @return event sample
     */
    int getSample() const;

    //=========================================================================================================
    /**
     * @brief setSample
     * @param iSample
     */
    void setSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event group
     *
     * @return event group
     */
    idNum getGroupId() const;

    //=========================================================================================================
    /**
     * @brief setGroupId
     * @param iGroup
     */
    void setGroupId(idNum iGroup);

    //=========================================================================================================
    /**
     * @brief getId
     * @return
     */
    idNum getId() const;

    //=========================================================================================================
    //**
    //**     * @brief getDescription
    //**     * @return
    //**     */
    std::string getDescription() const;

    //=========================================================================================================
    /**
     * @brief setDescription
     * @param description
     */
    void setDescription(const std::string& description);

    //=========================================================================================================
    /**
     * @brief setDescription
     * @param description
     */
    void setDescription(std::string&& description);

    //=========================================================================================================
    /**
     * @brief operator <
     * @param rhs
     * @return
     */
    bool operator<(const EventINT& rhs) const;

    //=========================================================================================================
    /**
     * @brief operator ==
     * @param rhs
     * @return
     */
    bool operator==(const EventINT& rhs) const;

    EventINT operator=(const EventINT& rhs);

private:
    idNum       m_iId;                      /**< Placeholder for sample Id */
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */
    idNum       m_iGroup;                   /**< Group this event belongs to */
    std::string m_sDescription;             /**< Short string describing info */
};

//=========================================================================================================
/**
 * @brief EventINT::fromSample
 * @param sample
 * @return
 */
inline EventINT EventINT::fromSample(int sample)
{
    return EventINT(0, sample, 0);
}

}//namespace EVENTSINTERNAL

//=========================================================================================================
/**
 *
 */
template<>
struct std::hash<EVENTSINTERNAL::EventINT>
{
    size_t operator()(const EVENTSINTERNAL::EventINT& rhs) const
    {
        return std::hash<int>()(rhs.getId());
    }
};


#endif // EVENT_H


