//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     time_frequency_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TimeFrequencyPanel chooses the trial and the row a time-frequency plot shows.
 */

#ifndef VIZ_TIME_FREQUENCY_PANEL_H
#define VIZ_TIME_FREQUENCY_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of a time-frequency plot: the trial and the row (channel) it shows.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TimeFrequencyPanel : public Panel
{
public:
    //=========================================================================================================
    /**
     * Constructs the panel with trial 0 and row 0.
     */
    TimeFrequencyPanel();

    ~TimeFrequencyPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callback   Called with the trial and the row when either changed.
     */
    void set_callback(std::function<void(int trial, int row)> callback)
    {
        m_callback = std::move(callback);
    }

    /** @return The trial. */
    int trial() const;

    /** @return The row. */
    int row() const;

private:
    std::function<void(int, int)> m_callback; /**< Called on changes. */
    int m_listener = 0;                       /**< Listener on the form. */
};

} // namespace mne::viz

#endif // VIZ_TIME_FREQUENCY_PANEL_H
