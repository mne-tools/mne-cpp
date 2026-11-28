//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     averaging_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    AveragingPanel sets the epochs, baseline and trials of an evoked average.
 */

#ifndef VIZ_AVERAGING_PANEL_H
#define VIZ_AVERAGING_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <utility>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffEvokedSet;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of the averaging: the stimulus channel and number of trials (real time), the epoch window,
 * the baseline correction within it, the detected trials per type, and the offline options to drop rejected
 * epochs, recompute live and save.
 *
 * The baseline stays inside the epoch window. The settings keep the keys of the former AveragingSettingsView;
 * the stimulus channel is kept by name until the channels arrive.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT AveragingPanel : public Panel
{
public:
    /** Reports of the panel; times are in milliseconds. */
    struct Callbacks
    {
        std::function<void(int)> pre_stim_changed;                    /**< The pre-stimulus time changed. */
        std::function<void(int)> post_stim_changed;                   /**< The post-stimulus time changed. */
        std::function<void(int)> baseline_from_changed;               /**< The baseline start changed. */
        std::function<void(int)> baseline_to_changed;                 /**< The baseline end changed. */
        std::function<void(int)> num_averages_changed;                /**< The number of trials changed. */
        std::function<void(const std::string&)> stim_channel_changed; /**< The stimulus channel changed. */
        std::function<void(bool)> baseline_active_changed;            /**< Baseline correction was switched. */
        std::function<void(bool)> drop_rejected_changed;              /**< Dropping rejected epochs was switched. */
        std::function<void(bool)> auto_compute_changed;               /**< Live recomputation was switched. */
        std::function<void()> reset;                                  /**< Reset was pressed. */
        std::function<void()> compute;                                /**< Save was pressed. */
    };

    //=========================================================================================================
    /**
     * @param[in] stimChannels   Stimulus channels by name with their indices.
     */
    explicit AveragingPanel(const std::map<std::string, int>& stimChannels = {});

    ~AveragingPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callbacks   The reports of the panel.
     */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Offers the stimulus channels; the stored one stays selected when present. An empty map is ignored.
     *
     * @param[in] stimChannels   Stimulus channels by name with their indices.
     */
    void set_stim_channels(const std::map<std::string, int>& stimChannels);

    //=========================================================================================================
    /**
     * Lists the averages of @p evokedSet with their number of trials, at most ten.
     *
     * @param[in] evokedSet   The current averages.
     */
    void set_detected_epochs(const mne::FiffEvokedSet& evokedSet);

    //=========================================================================================================
    /**
     * @return The stimulus channel chosen or stored.
     */
    const std::string& current_stim_channel() const
    {
        return m_stimChannel;
    }

    //=========================================================================================================
    /**
     * @return The index of the selected stimulus channel, or 0.
     */
    int stim_channel_index() const;

    //=========================================================================================================
    /**
     * @return Whether baseline correction is on.
     */
    bool baseline_correction() const;

    //=========================================================================================================
    /**
     * @return The number of trials per average.
     */
    int num_averages() const;

    //=========================================================================================================
    /**
     * @return Start of the baseline in ms.
     */
    int baseline_from() const;

    //=========================================================================================================
    /**
     * @return End of the baseline in ms.
     */
    int baseline_to() const;

    //=========================================================================================================
    /**
     * @return Time before the stimulus in ms.
     */
    int pre_stim() const;

    //=========================================================================================================
    /**
     * @return Time after the stimulus in ms.
     */
    int post_stim() const;

    //=========================================================================================================
    /**
     * @return Whether averages are recomputed live.
     */
    bool auto_compute() const;

    //=========================================================================================================
    /**
     * Restores the settings without reporting them and fits the baseline into the stored epoch window.
     *
     * @param[in] store   Settings store.
     * @param[in] path    Settings path.
     */
    void load(const SettingsStore& store, const std::string& path) override;

    //=========================================================================================================
    /**
     * @param[in] store   Settings store.
     * @param[in] path    Settings path.
     */
    void save(SettingsStore& store, const std::string& path) const override;

protected:
    void processing_mode_changed() override;

private:
    void changed(const std::string& key);
    void fill_stim_channels();

    Callbacks m_callbacks;                     /**< Reports of the panel. */
    std::map<std::string, int> m_stimChannels; /**< Stimulus channels with their indices. */
    std::string m_stimChannel = "STI 014";     /**< Chosen or stored stimulus channel. */
    int m_listener = 0;                        /**< Listener on the form. */
    bool m_updating = false;                   /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_AVERAGING_PANEL_H
