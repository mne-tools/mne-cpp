#ifndef MNE_EVENT_H
#define MNE_EVENT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Trigger event marker.
 */
class MNESHARED_EXPORT MneEvent
{
public:
    MneEvent() = default;
    ~MneEvent() = default;

    unsigned int from = 0;          /**< Source transition value. */
    unsigned int to = 0;            /**< Destination transition value. */
    int          sample = 0;        /**< Sample number. */
    int          show = 0;          /**< Can be used as desired. */
    int          created_here = 0;  /**< Was this event created in the program. */
    char         *comment = nullptr; /**< Event comment. */
};

/** Backward-compatible typedef aliases. */
typedef MneEvent  mneEventRec;
typedef MneEvent* mneEvent;

} // namespace MNELIB

#endif // MNE_EVENT_H
