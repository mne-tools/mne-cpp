#ifndef MNE_EVENT_LIST_H
#define MNE_EVENT_LIST_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"
#include "mne_event.h"

#include <memory>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Ordered list of trigger-event markers.
 *
 * Owns its MneEvent objects via unique_ptr.
 */
class MNESHARED_EXPORT MneEventList
{
public:
    MneEventList() = default;
    ~MneEventList() = default;

    //=========================================================================================================
    /**
     * @brief Returns the number of events in the list.
     */
    int nevent() const { return static_cast<int>(events.size()); }

    std::vector<std::unique_ptr<MneEvent>> events; /**< Owned event pointers. */
};

} // namespace MNELIB

#endif // MNE_EVENT_LIST_H
