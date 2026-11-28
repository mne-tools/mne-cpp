//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     artifact_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    ArtifactPanel sets the thresholds of the artifact rejection per channel type.
 */

#ifndef VIZ_ARTIFACT_PANEL_H
#define VIZ_ARTIFACT_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"
#include <mne/fiff/fiff_ch_info.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of the artifact rejection: whether it is active and, for each channel type present (grad,
 * mag, eeg, ecg, emg, eog), a peak-to-peak threshold as mantissa and decimal exponent.
 *
 * The thresholds go out as a map by channel type plus "Active" (1 or 0). The settings keep the keys of the former
 * ArtifactSettingsView.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT ArtifactPanel : public Panel
{
public:
    //=========================================================================================================
    /**
     * @param[in] channels   The channels whose types get a threshold.
     */
    explicit ArtifactPanel(const std::vector<mne::FiffChInfo>& channels = {});

    ~ArtifactPanel() override;

    //=========================================================================================================
    /**
     * @param[in] callback   Called with the thresholds when the user changes them or the channels change.
     */
    void set_thresholds_callback(std::function<void(const std::map<std::string, double>& thresholds)> callback)
    {
        m_callback = std::move(callback);
    }

    //=========================================================================================================
    /**
     * Shows the channel types of @p channels and reports the thresholds.
     *
     * @param[in] channels   The channels whose types get a threshold.
     */
    void set_channels(const std::vector<mne::FiffChInfo>& channels);

    //=========================================================================================================
    /**
     * @return The thresholds by channel type, and "Active".
     */
    std::map<std::string, double> thresholds() const;

    //=========================================================================================================
    /**
     * @return Whether rejection is active and a channel type has a threshold.
     */
    bool rejection_active() const;

    //=========================================================================================================
    /**
     * Restores the settings without reporting them.
     *
     * @param[in] store   Settings store.
     * @param[in] path    Settings path.
     */
    void load(const SettingsStore& store, const std::string& path) override;

private:
    void report();

    std::function<void(const std::map<std::string, double>&)> m_callback; /**< Called with the thresholds. */
    int m_listener = 0;                                                   /**< Listener on the form. */
    bool m_updating = false;                                              /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_ARTIFACT_PANEL_H
