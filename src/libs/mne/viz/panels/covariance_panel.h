//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     covariance_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CovariancePanel sets how many samples a covariance estimate uses.
 */

#ifndef VIZ_COVARIANCE_PANEL_H
#define VIZ_COVARIANCE_PANEL_H

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
 * @brief Tool panel of the covariance estimation: the number of samples, from one second to a minute of data in
 * steps of 600 samples.
 *
 * The number comes from the plugin and is not stored.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT CovariancePanel : public Panel
{
public:
    //=========================================================================================================
    /**
     * Constructs the panel with 600 samples.
     */
    CovariancePanel();

    ~CovariancePanel() override;

    //=========================================================================================================
    /**
     * @param[in] callback   Called with the number of samples the user chose.
     */
    void set_samples_callback(std::function<void(int samples)> callback)
    {
        m_callback = std::move(callback);
    }

    //=========================================================================================================
    /**
     * Sets the range of samples without a report; the current number is clamped into it.
     *
     * @param[in] samples   Samples of one second; a minute is the most.
     */
    void set_minimum_samples(int samples);

    //=========================================================================================================
    /**
     * @param[in] samples   Number of samples, without calling back.
     */
    void set_samples(int samples);

    //=========================================================================================================
    /**
     * @return The number of samples.
     */
    int samples() const;

private:
    std::function<void(int)> m_callback; /**< Called when the user chose a number. */
    int m_listener = 0;                  /**< Listener on the form. */
    bool m_updating = false;             /**< Whether the panel sets the number itself. */
};

} // namespace mne::viz

#endif // VIZ_COVARIANCE_PANEL_H
