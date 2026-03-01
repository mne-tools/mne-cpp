#ifndef MNE_EVENT_LIST_H
#define MNE_EVENT_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_event.h"

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

/**
 * Ordered list of trigger event markers.
 */
class MNESHARED_EXPORT MneEventList
{
public:
    MneEventList() = default;
    ~MneEventList() = default;

    MneEvent     **events = nullptr; /**< Array of events. */
    int          nevent = 0;         /**< Number of events. */
};

/** Backward-compatible typedef aliases. */
typedef MneEventList  mneEventListRec;
typedef MneEventList* mneEventList;

} // namespace MNELIB

#endif // MNE_EVENT_LIST_H
