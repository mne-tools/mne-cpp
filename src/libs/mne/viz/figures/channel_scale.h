//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     channel_scale.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Amplitude scales of channels by kind and unit.
 */

#ifndef VIZ_CHANNEL_SCALE_H
#define VIZ_CHANNEL_SCALE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <map>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * The default amplitude scale of a channel: 1e-12 T for magnetometers, 1e-15 T/m for gradiometers, 1e-5 V for
 * EEG and so on, 1e-9 for unknown kinds.
 *
 * @param[in] kind   Channel kind, e.g. FIFFV_MEG_CH.
 * @param[in] unit   Channel unit, which tells MEG gradiometers (FIFF_UNIT_T_M) from magnetometers (FIFF_UNIT_T).
 *
 * @return The value drawn at full scale.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT float default_channel_scale(int kind, int unit);

//=============================================================================================================
/**
 * The amplitude scale of a channel from a scale map: keyed by the unit for MEG channels and by the kind for all
 * others, falling back to default_channel_scale() for missing, zero or NaN entries.
 *
 * @param[in] scales   Scales by channel kind, and by unit for MEG.
 * @param[in] kind     Channel kind.
 * @param[in] unit     Channel unit.
 *
 * @return The value drawn at full scale.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT float channel_scale(const std::map<int, float>& scales, int kind, int unit);

} // namespace mne::viz

#endif // VIZ_CHANNEL_SCALE_H
