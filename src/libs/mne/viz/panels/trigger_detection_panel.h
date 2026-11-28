//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     trigger_detection_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    TriggerDetectionPanel sets up the detection of triggers on a stimulus channel.
 */

#ifndef VIZ_TRIGGER_DETECTION_PANEL_H
#define VIZ_TRIGGER_DETECTION_PANEL_H

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
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of the trigger detection: the stimulus channel, the threshold as mantissa and decimal
 * exponent, and in real-time mode the activation, a colour per detected trigger type and the number of detected
 * triggers; offline it offers to detect the triggers once.
 *
 * The settings keep the keys of the former TriggerDetectionView (Activated, ChannelIndex, FirstThresholdValue,
 * SecondThresholdValue); the colours are stored as "#rrggbb" under Colors/ followed by the trigger type.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT TriggerDetectionPanel : public Panel
{
public:
    /** Reports of the panel. */
    struct Callbacks
    {
        /** The detection changed: colours by trigger type, whether it is active, the channel and the threshold. */
        std::function<void(const std::map<double, std::string>& colors, bool active, const std::string& channel,
                           double threshold)>
            trigger_info_changed;
        std::function<void()> reset_trigger_counter;                                       /**< Reset was pressed. */
        std::function<void(const std::string& channel, double threshold)> detect_triggers; /**< Detect was pressed. */
    };

    //=========================================================================================================
    /**
     * Constructs the panel for real-time data; it reports once init() gave it the measurement info.
     */
    TriggerDetectionPanel();

    ~TriggerDetectionPanel() override;

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
     * Offers the stimulus channels of @p info, selects the stored one and reports the detection settings.
     *
     * @param[in] info   Measurement info.
     */
    void init(const mne::FiffInfo& info);

    //=========================================================================================================
    /**
     * Shows the number of detected triggers and adds the new trigger types to the colour choice.
     *
     * @param[in] total      Number of detected triggers.
     * @param[in] triggers   Detected triggers by channel as (sample, value) pairs; the values are the types.
     */
    void set_detected_triggers(int total, const std::map<int, std::vector<std::pair<int, double>>>& triggers);

    //=========================================================================================================
    /**
     * @return The selected stimulus channel, or an empty name.
     */
    std::string selected_stim_channel() const;

    //=========================================================================================================
    /**
     * @return The threshold, mantissa times ten to the exponent.
     */
    double threshold() const;

    //=========================================================================================================
    /**
     * @return Whether the real-time detection is active.
     */
    bool active() const;

    //=========================================================================================================
    /**
     * @return The colours ("#rrggbb") by trigger type.
     */
    const std::map<double, std::string>& colors() const
    {
        return m_colors;
    }

    //=========================================================================================================
    /**
     * Restores the settings without reporting them.
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
    void report_info();
    void show_type_color();

    Callbacks m_callbacks;                  /**< Reports of the panel. */
    std::map<double, std::string> m_colors; /**< Colours by trigger type. */
    int m_storedChannel = 0;                /**< Index of the stored channel among the stimulus channels. */
    int m_listener = 0;                     /**< Listener on the form. */
    bool m_initialized = false;             /**< Whether init() was called. */
    bool m_updating = false;                /**< Whether the panel changes fields itself. */
};

} // namespace mne::viz

#endif // VIZ_TRIGGER_DETECTION_PANEL_H
