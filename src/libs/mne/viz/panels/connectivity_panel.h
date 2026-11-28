//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     connectivity_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ConnectivityPanel chooses the connectivity metric, window, trials, trigger type and frequency band.
 */

#ifndef VIZ_CONNECTIVITY_PANEL_H
#define VIZ_CONNECTIVITY_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <string>
#include <vector>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class SettingsStore;

//=============================================================================================================
/**
 * @brief Tool panel of the functional connectivity: the metric (COR, COH, IMAGCOH, XCOR, PLI, DSWPLI, USPLI, WPLI,
 * PLV), the window type, the number of trials, the trigger type and the frequency band.
 *
 * The settings are stored under "<path>/ConnectivitySettingsView/<key>" and restored without reports, so hosts
 * read them once after the restore.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ConnectivityPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(const std::string& metric)> metric_changed;      /**< The metric changed. */
        std::function<void(const std::string& window)> window_type_changed; /**< The window type changed. */
        std::function<void(int trials)> number_trials_changed;              /**< The number of trials changed. */
        std::function<void(const std::string& type)> trigger_type_changed;  /**< The trigger type changed. */
        std::function<void(float low, float high)> frequency_band_changed;  /**< A band edge changed. */
    };

    //=========================================================================================================
    /**
     * Constructs the panel: COR, Hanning window, 10 trials, 7 to 13 Hz.
     */
    ConnectivityPanel();

    ~ConnectivityPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callbacks   What to call on changes.
     */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Adds trigger types the panel does not offer yet.
     *
     * @param[in] types   Trigger types.
     */
    void set_trigger_types(const std::vector<std::string>& types);

    //=========================================================================================================
    /**
     * @param[in] trials   Number of trials, without a report.
     */
    void set_number_trials(int trials);

    //=========================================================================================================
    /**
     * @param[in] low    Lower edge in Hz.
     * @param[in] high   Upper edge in Hz.
     */
    void set_frequency_band(double low, double high);

    /** @return The metric. */
    std::string metric() const;

    /** @return The window type. */
    std::string window_type() const;

    /** @return The number of trials. */
    int number_trials() const;

    /** @return The trigger type. */
    std::string trigger_type() const;

    /** @return The lower edge of the band in Hz. */
    double lower_frequency() const;

    /** @return The upper edge of the band in Hz. */
    double upper_frequency() const;

    //=========================================================================================================
    /**
     * Restores the settings without reporting them.
     *
     * @param[in] store   Stored settings.
     * @param[in] path    Settings path of the host.
     */
    void load(const SettingsStore& store, const std::string& path) override;

private:
    void changed(const std::string& key);

    Callbacks m_callbacks;   /**< What to call on changes. */
    int m_trials = 10;       /**< The reported number of trials. */
    int m_listener = 0;      /**< Listener on the form. */
    bool m_updating = false; /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_CONNECTIVITY_PANEL_H
