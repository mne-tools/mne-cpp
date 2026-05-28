//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_event_list.h
 * @since March 2026
 * @brief Ordered list of @ref MNELIB::MNEEvent records.
 *
 * @ref MNELIB::MNEEventList wraps the per-recording event list with the
 * helpers needed by epoching (search by id, slice by time window) and
 * by FIFF / ASCII round-trip with the @c .eve format.
 */

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
