//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     filter_settings_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    FilterSettingsPanel switches filtering and edits the pass band of its FilterDesignPanel.
 */

#ifndef VIZ_FILTER_SETTINGS_PANEL_H
#define VIZ_FILTER_SETTINGS_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "filter_design_panel.h"
#include "panel.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <functional>
#include <memory>
#include <string>
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Compact tool panel of filtering: whether it is active, the channel type and the pass band, with an
 * action that asks the host to show the full FilterDesignPanel.
 *
 * The panel owns its design panel. Edits of the channel type and the edges go to the design panel as if the user
 * made them there, so its callbacks report the new kernel; the design panel's edges and channel type show here.
 * load() and save() include the design panel's settings. In clinical mode the design action is hidden.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT FilterSettingsPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(bool active)> activation_changed; /**< Filtering was switched. */
        std::function<void()> show_design;                   /**< The user asked for the full filter design. */
    };

    static constexpr const char* active_key = "filterActivated";         /**< Key of the activation. */
    static constexpr const char* channel_type_key = "filterChannelType"; /**< Key of the channel type. */
    static constexpr const char* from_key = "filterFrom";                /**< Key of the lower edge in Hz. */
    static constexpr const char* to_key = "filterTo";                    /**< Key of the upper edge in Hz. */
    static constexpr const char* show_design_key = "showFilterOptions";  /**< Key of the design action. */

    //=========================================================================================================
    /**
     * Constructs the panel, inactive, with a new FilterDesignPanel.
     */
    FilterSettingsPanel();

    ~FilterSettingsPanel() override;

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
     * @return The design panel, for the host to show it and to set its callbacks.
     */
    const std::shared_ptr<FilterDesignPanel>& design() const
    {
        return m_design;
    }

    //=========================================================================================================
    /**
     * @return Whether filtering is active.
     */
    bool active() const;

    //=========================================================================================================
    /**
     * Limits the edges to the Nyquist frequency and designs the kernel for @p sfreq; silent.
     *
     * @param[in] sfreq   Sampling frequency in Hz.
     */
    void set_sampling_rate(double sfreq);

    //=========================================================================================================
    /**
     * Restores the settings of the panel and its design panel without reports.
     *
     * @param[in] store   Settings to read.
     * @param[in] path    Settings path of the host.
     */
    void load(const SettingsStore& store, const std::string& path) override;

    //=========================================================================================================
    /**
     * Stores the settings of the panel and its design panel.
     *
     * @param[in, out] store   Settings to write.
     * @param[in]      path    Settings path of the host.
     */
    void save(SettingsStore& store, const std::string& path) const override;

protected:
    void gui_mode_changed() override;

private:
    void changed(const std::string& key);
    void show_design_state();

    Callbacks m_callbacks;                       /**< What to call on changes. */
    std::shared_ptr<FilterDesignPanel> m_design; /**< The full filter design. */
    int m_listener = 0;                          /**< Listener on the form. */
    int m_designListener = 0;                    /**< Listener on the design panel's form. */
    bool m_updating = false;                     /**< Whether the panel sets fields itself. */
};

} // namespace mne::viz

#endif // VIZ_FILTER_SETTINGS_PANEL_H
