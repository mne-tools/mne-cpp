//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     acq_timebase.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.5.0
 * @date     November 2026
 * @brief    Definition of verify() declared in @ref acq_timebase.h.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "acq_timebase.h"

#include <algorithm>
#include <limits>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

mne::AcqExpected<mne::AcqTimebase> mne::verify(const AcqTimebase& target, std::span<const AcqSyncMarker> markers,
                                               const AcqTimebase& reference, std::uint64_t toleranceTicks,
                                               std::uint64_t minSpanSamples) noexcept
{
    if (reference.domain == 0 || target.domain != reference.domain || reference.lock != AcqLockState::PhaseLocked ||
        target.lock == AcqLockState::Free) {
        return AcqErrorCode::NotSynchronized;
    }
    const std::size_t needed = target.lock == AcqLockState::Declared ? 2 : 1;
    if (markers.size() < needed) {
        return AcqErrorCode::NotSynchronized;
    }

    std::int64_t lowest = std::numeric_limits<std::int64_t>::max();
    std::int64_t highest = std::numeric_limits<std::int64_t>::min();
    std::uint64_t firstSample = std::numeric_limits<std::uint64_t>::max();
    std::uint64_t lastSample = 0;
    for (const AcqSyncMarker& m : markers) {
        const std::int64_t origin = reference.tickOf(m.referenceSample) + m.subTicks -
            static_cast<std::int64_t>(m.targetSample * target.ticksPerSample);
        lowest = std::min(lowest, origin);
        highest = std::max(highest, origin);
        firstSample = std::min(firstSample, m.targetSample);
        lastSample = std::max(lastSample, m.targetSample);
    }
    if (target.lock == AcqLockState::Declared && lastSample - firstSample < minSpanSamples) {
        return AcqErrorCode::NotSynchronized;
    }
    const auto spread = static_cast<std::uint64_t>(highest - lowest);
    if (spread > toleranceTicks) {
        return AcqErrorCode::NotSynchronized;
    }

    AcqTimebase result = target;
    result.originTick = lowest + static_cast<std::int64_t>(spread / 2);
    result.originUncertaintyTicks = spread;
    result.lock = AcqLockState::PhaseLocked;
    return result;
}
