//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     hpi_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    HpiPanel sets up head position indicator (HPI) fitting and shows its results.
 */

#ifndef VIZ_HPI_PANEL_H
#define VIZ_HPI_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"
#include <mne/base/json.h>
#include <mne/fiff/fiff_dig_point_set.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

class SettingsStore;

//=============================================================================================================
/**
 * @brief Tool panel of the HPI fitting: the digitizers with the HPI coils, the coil frequencies (typed or from a
 * preset), the fitting options, and the fit errors, goodness of fit and head movement.
 *
 * The coil frequencies are a comma-separated list in Hz; "none" marks a coil without a frequency. The coil
 * frequencies, the SSP, compensator and continuous fitting switches, the error threshold and the fitting window are
 * stored under "<path>/HpiSettingsView/<key>" and restored without reports. Programmatic changes are not reported;
 * the user's are.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT HpiPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(const std::vector<int>& frequencies)> coil_frequencies_changed; /**< Coil frequencies in Hz, -1 for none. */
        std::function<void(const mne::FiffDigPointSet& digitizers, const std::filesystem::path& file)>
            digitizers_changed;                                           /**< Digitizers were loaded from a file. */
        std::function<void()> order_frequencies;                          /**< "Order coil frequencies" was clicked. */
        std::function<void()> single_fit;                                 /**< "Perform single HPI fit" was clicked. */
        std::function<void(bool on)> ssp_changed;                         /**< SSP was switched. */
        std::function<void(bool on)> compensators_changed;                /**< The compensators were switched. */
        std::function<void(bool on)> continuous_fitting_changed;          /**< Continuous fitting was switched. */
        std::function<void(int samples)> fitting_window_size_changed;     /**< The fitting window changed. */
        std::function<void(double millimeters)> allowed_error_changed;    /**< The allowed mean error changed. */
        std::function<void(double millimeters)> allowed_movement_changed; /**< The allowed head movement changed. */
        std::function<void(double degrees)> allowed_rotation_changed;     /**< The allowed head rotation changed. */
    };

    /** Creates the panel with the Neuromag VectorView presets and four coils at 293, 307, 314 and 321 Hz. */
    HpiPanel();

    ~HpiPanel() override;

    /** @param[in] callbacks   What the panel reports. */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Shows the fit error of each coil and their mean, and whether the fit is good.
     *
     * @param[in] errors      Error of each coil in m.
     * @param[in] meanError   Mean error in m; above the error threshold the fit is bad.
     */
    void set_errors(const std::vector<double>& errors, double meanError);

    //=========================================================================================================
    /**
     * Shows the goodness of fit of each coil and their mean.
     *
     * @param[in] goodness       Goodness of fit of each coil, 0 to 1.
     * @param[in] meanGoodness   Mean goodness of fit.
     */
    void set_goodness_of_fit(const Eigen::VectorXd& goodness, double meanGoodness);

    //=========================================================================================================
    /**
     * Shows the head movement since the last reference position and whether it is big.
     *
     * @param[in] movement   Movement in m.
     * @param[in] rotation   Rotation in degrees.
     */
    void set_movement(double movement, double rotation);

    //=========================================================================================================
    /**
     * Shows the digitizers: the number of points of each kind, one coil per HPI point, and the presets for that
     * number of coils.
     *
     * @param[in] digitizers   The digitizer points.
     */
    void set_digitizers(const mne::FiffDigPointSet& digitizers);

    //=========================================================================================================
    /**
     * Loads the digitizers of a FIFF file as the user does, reporting them and stopping continuous fitting.
     *
     * @param[in] file   The FIFF file.
     *
     * @return Whether digitizers were loaded.
     */
    bool load_digitizers(const std::filesystem::path& file);

    //=========================================================================================================
    /**
     * Replaces the coil presets, by default the Neuromag VectorView frequencies, with those of a JSON file: an
     * object mapping the number of coils ("4", "5", ...) to an array of {"name": ..., "coils": [Hz, ...]}.
     *
     * @param[in] file   The JSON file.
     *
     * @return Whether the file could be read and parsed; if not, the presets are left unchanged.
     */
    bool load_coil_presets(const std::filesystem::path& file);

    //=========================================================================================================
    /**
     * Sets the coil frequencies.
     *
     * @param[in] frequencies   Frequencies in Hz, -1 for a coil without a frequency.
     */
    void set_coil_frequencies(const std::vector<int>& frequencies);

    /** @return The coil frequencies in Hz, -1 for a coil without a frequency. */
    const std::vector<int>& coil_frequencies() const
    {
        return m_frequencies;
    }

    /** @return The names of the coil presets for the current number of coils. */
    std::vector<std::string> coil_presets() const;

    /** @return Whether SSP is applied. */
    bool use_ssp() const;

    /** @return Whether the compensators are applied. */
    bool use_compensators() const;

    /** @return Whether the head position is fitted continuously. */
    bool continuous_fitting() const;

    /** @return The fitting window in samples. */
    int fitting_window_size() const;

    /** @return The allowed mean error in mm. */
    double allowed_error() const;

    /** @return The allowed head movement in mm. */
    double allowed_movement() const;

    /** @return The allowed head rotation in degrees. */
    double allowed_rotation() const;

    //=========================================================================================================
    /**
     * Restores the panel without reports; a stored list without valid frequencies keeps the current one.
     *
     * @param[in] store   Stored settings.
     * @param[in] path    Settings path of the host; empty restores nothing.
     */
    void load(const SettingsStore& store, const std::string& path) override;

    //=========================================================================================================
    /**
     * Parses a comma- or space-separated list of coil frequencies.
     *
     * @param[in] text   The list, e.g. "293, 307, none, 321".
     *
     * @return The frequencies in Hz, -1 for each entry that is no positive number.
     */
    static std::vector<int> parse_frequencies(const std::string& text);

    //=========================================================================================================
    /**
     * @param[in] frequencies   Frequencies in Hz, -1 for none.
     *
     * @return The frequencies as the panel shows them, e.g. "293, 307, none, 321".
     */
    static std::string format_frequencies(const std::vector<int>& frequencies);

private:
    void changed(const std::string& key);
    void show_frequencies();
    void setup_presets();
    void set_info(const std::string& key, const std::string& text, SettingField::Tone tone = SettingField::Tone::Normal);
    void report_frequencies() const;

    Callbacks m_callbacks;               /**< What to call on changes. */
    std::vector<int> m_frequencies;      /**< Coil frequencies in Hz, -1 for none. */
    std::vector<std::string> m_errors;   /**< Shown error of each coil. */
    std::vector<std::string> m_goodness; /**< Shown goodness of fit of each coil. */
    mne::JsonObject m_presets;           /**< Coil frequency presets by number of coils. */
    int m_listener = 0;                  /**< Listener on the form. */
    bool m_updating = false;             /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_HPI_PANEL_H
