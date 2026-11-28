//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     dipole_fit_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    DipoleFitPanel sets up an equivalent current dipole fit: measurement, time scale, noise, forward model.
 */

#ifndef VIZ_DIPOLE_FIT_PANEL_H
#define VIZ_DIPOLE_FIT_PANEL_H

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
#include <utility>

//=============================================================================================================
// DEFINE NAMESPACE mne::viz
//=============================================================================================================

namespace mne::viz
{

//=============================================================================================================
/**
 * @brief Tool panel of the dipole fit: the measurement and data set, the modalities, the time scale, the baseline,
 * the fitting guesses, the noise (manual, regularization or a noise covariance file), the forward model (BEM, MRI
 * coordinates or a sphere model) and the name of the fit.
 *
 * Models are chosen by name; "None" chooses none. Choosing a measurement names the fit after it. Programmatic
 * changes are not reported; the user's are. Nothing is stored.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT DipoleFitPanel : public Panel
{
public:
    /** Kinds of models to choose from, numbered as by the dipole fit plugin. */
    enum class Model
    {
        Measurement = 1, /**< Raw or averaged data. */
        Bem = 2,         /**< A boundary element model. */
        Mri = 3,         /**< MRI coordinates. */
        Noise = 4        /**< A noise covariance. */
    };

    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(const std::string& name)> fit;                                /**< "Fit" was clicked. */
        std::function<void(int set)> set_changed;                                        /**< The data set changed. */
        std::function<void(bool eeg, bool meg)> modality_changed;                        /**< A modality was switched. */
        std::function<void(int tmin, int tmax, int tstep, int tint)> time_changed;       /**< The time scale changed, in ms. */
        std::function<void(int bmin, int bmax)> baseline_changed;                        /**< The baseline changed, in ms; 1e6 twice for none. */
        std::function<void(double minDistance, double radius)> fitting_changed;          /**< The fitting guesses changed, in mm. */
        std::function<void(double grad, double mag, double eeg)> noise_changed;          /**< The manual noise changed. */
        std::function<void(double grad, double mag, double eeg)> regularization_changed; /**< The regularization changed. */
        std::function<void(double x, double y, double z, double radius)> sphere_changed; /**< The sphere model changed, in mm. */
        std::function<void(Model kind, const std::string& name)> model_changed;          /**< Another model was chosen. */
    };

    /** Creates the panel with MNE-C's defaults, e.g. a sphere model at (0, 0, 40) mm with a radius of 90 mm. */
    DipoleFitPanel();

    ~DipoleFitPanel() override;

    /** @param[in] callbacks   What the panel reports. */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Adds a model to choose from.
     *
     * @param[in] kind   Kind of the model.
     * @param[in] name   File name of the model.
     */
    void add_model(Model kind, const std::string& name);

    //=========================================================================================================
    /**
     * Removes a model; if it was chosen, "None" is chosen.
     *
     * @param[in] kind   Kind of the model.
     * @param[in] name   File name of the model.
     */
    void remove_model(Model kind, const std::string& name);

    //=========================================================================================================
    /**
     * @param[in] kind   Kind of the models.
     *
     * @return The chosen model, "None" for none.
     */
    std::string model(Model kind) const;

    /** Reports all parameters except the models, e.g. so a new host starts from them. */
    void report_parameters() const;

    /** @return The name of the fit. */
    std::string fit_name() const;

    //=========================================================================================================
    /**
     * @param[in] measurement   File name of a measurement, e.g. "sample_audvis-ave.fif".
     *
     * @return The base name of the measurement without extension and "-ave", "_ave", "-raw" or "_raw" suffix;
     *         empty for a name without any.
     */
    static std::string measurement_name(const std::string& measurement);

private:
    void changed(const std::string& key);
    void name_fit();
    void report_modality() const;
    void report_time() const;
    void report_baseline() const;
    void report_fitting() const;
    void report_noise() const;
    void report_regularization() const;
    void report_sphere() const;

    Callbacks m_callbacks;   /**< What to call on changes. */
    int m_listener = 0;      /**< Listener on the form. */
    bool m_updating = false; /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_DIPOLE_FIT_PANEL_H
