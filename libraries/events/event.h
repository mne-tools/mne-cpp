#ifndef EVENT_EVENTSINTERNAL_H
#define EVENT_EVENTSINTERNAL_H

namespace EVENTSINTERNAL {

class EventGroup;

class Event
{
public:
    //=========================================================================================================
    /**
     * Create an event at sample iSample
     *
     * @param iSample
     * @param group
     * @param iType
     */
    Event(int iSample, const EventGroup& group);

    //=========================================================================================================
    /**
     * Returns event sample
     *
     * @return event sample
     */
    int getSample() const;

    //=========================================================================================================
    void setSample(int iSample);

    //=========================================================================================================
    /**
     * Returns event group
     *
     * @return event group
     */
    int getGroup() const;

    //=========================================================================================================
    void setGroup(int iGroup);

    int getId() const;

    bool operator<(const Event& rhs) const;

private:
    int         m_iSample;                  /**< Sample coorespodning to the instantaneous event */
    int         m_iGroup;                   /**< Group the event belongs to */
    int         m_iId;                      /**< Sample Id */

    static int eventIdCounter;
};

}
#endif // EVENT_H


