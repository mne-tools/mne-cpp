//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutmaker.h
 * @author   Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     November 2014
 * @brief    Generator that projects 3-D electrode positions onto a 2-D @c .lout topographic layout.
 *
 * @ref mne::LayoutMaker is the counterpart of
 * @ref mne::LayoutLoader "LayoutLoader": given the 3-D coordinates of an
 * EEG cap (typically captured with the Polhemus digitizer or
 * loaded from a vendor @c .elc file) it produces the 2-D
 * channel layout used by every topographic widget in
 * mne-cpp — butterfly plots, sensor maps, time-frequency
 * heatmaps in mne_viz_qt and mne_viz_qt.
 *
 * As in MNE-C @c mne_make_eeg_layout, a sphere is fitted to the
 * points (@ref mne::Sphere::fit_sphere_simplex) and every
 * electrode is mapped by an azimuthal equidistant projection
 * around the sphere apex. Optional mirroring flags exist for
 * caps whose left/right convention differs from MNE's defaults.
 */

#ifndef CHANNELS_LAYOUTMAKER_H
#define CHANNELS_LAYOUTMAKER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channels_global.h"
#include <mne/base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <optional>
#include <span>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief How @ref LayoutMaker::make_layout projects and sizes the channel boxes.
 *
 * @snippet ex_channels/main.cpp layout_maker_usage
 */
struct CHANNELSSHARED_EXPORT LayoutOptions
{
    bool fit_sphere = true;  /**< Project around the best-fitting sphere, else around the origin. */
    double radius = 20.0;    /**< Radius of the projection disk, in layout units. */
    double box_width = 5.0;  /**< Width of each channel box, in layout units. */
    double box_height = 4.0; /**< Height of each channel box, in layout units. */
    bool mirror_x = false;   /**< Negate the x coordinates. */
    bool mirror_y = false;   /**< Negate the y coordinates. */
};

//=============================================================================================================
/**
 * Makes 2-D channel layouts from 3-D sensor positions.
 *
 * @brief Makes 2-D channel layouts from 3-D sensor positions.
 *
 * @snippet ex_channels/main.cpp layout_maker_usage
 */
class CHANNELSSHARED_EXPORT LayoutMaker
{
public:
    //=========================================================================================================
    /**
     * Projects 3-D positions onto the plane.
     *
     * @param[in] positions  The sensor positions.
     * @param[in] options    Projection and box size.
     *
     * @return The lower-left corner of each channel box, or no value if there are no positions or all coincide.
     */
    static std::optional<std::vector<Eigen::Vector2d>> make_layout(std::span<const Eigen::Vector3d> positions,
                                                                   const LayoutOptions& options = {});

    //=========================================================================================================
    /**
     * Writes channel boxes as an MNE-C @c .lout file.
     *
     * @param[in] path     The output file.
     * @param[in] corners  The lower-left corner of each box, as returned by make_layout().
     * @param[in] names    The channel names; boxes without a name are written unnamed.
     * @param[in] options  The box size.
     *
     * @return Whether the file was written.
     */
    static bool write_lout(const std::filesystem::path& path,
                           std::span<const Eigen::Vector2d> corners,
                           const mne::StringList& names,
                           const LayoutOptions& options = {});
};

} // namespace mne

#endif // CHANNELS_LAYOUTMAKER_H
