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
 * Owns its MNEEvent objects via unique_ptr.
 */
class MNESHARED_EXPORT MNEEventList
{
public:
    MNEEventList() = default;
    ~MNEEventList() = default;

    MNEEventList(const MNEEventList& other)
    {
        events.reserve(other.events.size());
        for (const auto& e : other.events)
            events.push_back(std::make_unique<MNEEvent>(*e));
    }

    MNEEventList& operator=(const MNEEventList& other)
    {
        if (this != &other) {
            events.clear();
            events.reserve(other.events.size());
            for (const auto& e : other.events)
                events.push_back(std::make_unique<MNEEvent>(*e));
        }
        return *this;
    }

    MNEEventList(MNEEventList&&) = default;
    MNEEventList& operator=(MNEEventList&&) = default;

    //=========================================================================================================
    /**
     * @brief Returns the number of events in the list.
     */
    int nevent() const { return static_cast<int>(events.size()); }

    std::vector<std::unique_ptr<MNEEvent>> events; /**< Owned event pointers. */
};

} // namespace MNELIB

#endif // MNE_EVENT_LIST_H
