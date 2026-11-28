//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     coregistration_panel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    3.0.0
 * @date     November 2026
 * @brief    CoregistrationPanel co-registers the MRI and the head coordinate frame: fiducials, digitizers, ICP
 *           and a manual adjustment of the transformation.
 */

#ifndef VIZ_COREGISTRATION_PANEL_H
#define VIZ_COREGISTRATION_PANEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../viz_global.h"
#include "panel.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <array>
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

//=============================================================================================================
/**
 * @brief Tool panel of the MRI-head co-registration: the head surface (BEM), the MRI fiducials picked in a 3D view
 * or loaded from a file, the digitizers, the weights and options of the fiducial and ICP fits, and the scaling,
 * rotation and translation of the transformation.
 *
 * Files to load or store are chosen in path fields; a store path is cleared after it was reported, so the same file
 * can be stored again. Programmatic changes are not reported; the user's are. Nothing is stored.
 *
 * @since 3.0.0
 */
class VIZSHARED_EXPORT CoregistrationPanel : public Panel
{
public:
    /** What the panel reports. */
    struct Callbacks
    {
        std::function<void(const std::string& name)> bem_changed;                     /**< Another BEM was chosen. */
        std::function<void(const std::filesystem::path& file)> fiducials_loaded;      /**< Fiducials are to be loaded. */
        std::function<void(const std::filesystem::path& file)> fiducials_stored;      /**< Fiducials are to be stored. */
        std::function<void(const std::filesystem::path& file)> digitizers_loaded;     /**< Digitizers are to be loaded. */
        std::function<void(const std::filesystem::path& file)> transformation_loaded; /**< A transformation is to be loaded. */
        std::function<void(const std::filesystem::path& file)> transformation_stored; /**< The transformation is to be stored. */
        std::function<void()> fit_fiducials;                                          /**< "Fit Fiducials" was clicked. */
        std::function<void()> fit_icp;                                                /**< "Fit ICP" was clicked. */
        std::function<void()> transformation_changed;                                 /**< Scaling, rotation or translation were edited. */
        std::function<void(bool on)> picking_changed;                                 /**< Picking fiducials in the 3D view was switched. */
        std::function<void(int fiducial)> fiducial_changed;                           /**< Another fiducial (FIFFV_POINT_LPA, ...) is picked. */
    };

    /** Creates the panel without BEMs, picking the LPA first. */
    CoregistrationPanel();

    ~CoregistrationPanel() override;

    /** @param[in] callbacks   What the panel reports. */
    void set_callbacks(Callbacks callbacks)
    {
        m_callbacks = std::move(callbacks);
    }

    //=========================================================================================================
    /**
     * Sets the BEMs to choose from; the chosen one stays if it is still there, else the first one is chosen.
     *
     * @param[in] names   Names of the BEMs; none shows "Select BEM".
     */
    void set_bems(const std::vector<std::string>& names);

    /** @return The chosen BEM; empty for none. */
    std::string bem() const;

    //=========================================================================================================
    /**
     * Stores the position of the fiducial being picked and steps to the next one (LPA, nasion, RPA, LPA, ...).
     *
     * @param[in] position   Position in m.
     */
    void set_fiducial(const Eigen::Vector3f& position);

    /** @return The fiducial being picked: FIFFV_POINT_LPA, FIFFV_POINT_NASION or FIFFV_POINT_RPA. */
    int fiducial() const;

    /** @return Whether fiducials are picked in the 3D view. */
    bool picking() const;

    //=========================================================================================================
    /**
     * Shows the number of digitizers left out of the ICP fit.
     *
     * @param[in] points   Number of points.
     */
    void set_omitted_points(int points);

    //=========================================================================================================
    /**
     * Shows the root mean square distance between the digitizers and the head surface after the ICP fit.
     *
     * @param[in] rmse   Distance in m.
     */
    void set_rmse(float rmse);

    /** @return The maximum number of ICP iterations. */
    int max_iterations() const;

    /** @return The convergence limit of ICP in m. */
    float convergence() const;

    /** @return Whether the fiducial fit scales uniformly. */
    bool auto_scale() const;

    /** @return The maximum distance in m of digitizers from the head surface; farther ones are left out. */
    float omit_distance() const;

    /** @return The weight of the LPA. */
    float weight_lpa() const;

    /** @return The weight of the RPA. */
    float weight_rpa() const;

    /** @return The weight of the nasion. */
    float weight_nasion() const;

    /** @return The weight of the EEG points. */
    float weight_eeg() const;

    /** @return The weight of the head shape points. */
    float weight_hsp() const;

    /** @return The weight of the HPI points. */
    float weight_hpi() const;

    /** @return The kinds of digitizers (FIFFV_POINT_CARDINAL and the chosen EEG, HPI, EXTRA) used for ICP. */
    std::vector<int> digitizer_kinds() const;

    //=========================================================================================================
    /**
     * The transformation set in the panel.
     *
     * @param[out] rotation      Rotation angles in rad around z, y and x (Euler order).
     * @param[out] translation   Translation in m.
     * @param[out] scaling       Scaling along x, y and z, from the scaling mode.
     */
    void transformation(Eigen::Vector3f& rotation, Eigen::Vector3f& translation, Eigen::Vector3f& scaling) const;

    //=========================================================================================================
    /**
     * Shows a transformation, e.g. a fitted one.
     *
     * @param[in] translation   Translation in m.
     * @param[in] rotation      Rotation angles in rad around z, y and x (Euler order).
     * @param[in] scaling       Scaling along x, y and z.
     */
    void set_transformation(const Eigen::Vector3f& translation, const Eigen::Vector3f& rotation, const Eigen::Vector3f& scaling);

private:
    void changed(const std::string& key);
    void report_path(const std::string& key, const std::function<void(const std::filesystem::path&)>& callback, bool clear);
    void stop_picking();
    void show_fiducial();
    void update_scaling_mode();
    void set_silently(const std::string& key, const SettingValue& value);

    Callbacks m_callbacks;                      /**< What to call on changes. */
    std::array<Eigen::Vector3f, 3> m_fiducials; /**< Picked LPA, nasion and RPA in m. */
    int m_listener = 0;                         /**< Listener on the form. */
    bool m_updating = false;                    /**< Whether changes are not reported. */
};

} // namespace mne::viz

#endif // VIZ_COREGISTRATION_PANEL_H
