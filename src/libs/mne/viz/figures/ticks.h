//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     ticks.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    Axis ticks at round numbers and their labels.
 */

#ifndef VIZ_TICKS_H
#define VIZ_TICKS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

/**
 * Tick positions at multiples of 1, 2 or 5 times a power of ten.
 *
 * @param[in] lo        Lower end of the axis.
 * @param[in] hi        Upper end of the axis.
 * @param[in] target    About how many intervals the axis should get.
 * @param[in] minStep   Smallest allowed distance between ticks, e.g. 1 for counts.
 *
 * @return The ticks in [lo, hi], ascending; none if hi <= lo or target < 1.
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::vector<double> nice_ticks(double lo, double hi, int target, double minStep = 0.0);

/**
 * @param[in] value   Tick value.
 *
 * @return The label of a tick: up to six significant digits, as QString::number(value, 'g', 6).
 *
 * @since 3.0.0
 */
VIZSHARED_EXPORT std::string tick_label(double value);

} // namespace mne::viz

#endif // VIZ_TICKS_H
