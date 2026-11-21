//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     layoutloader.h
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2014
 * @brief    Reader for ANT/ASA @c .elc electrode files used by the topographic plotting widgets.
 *
 * ANT @c .elc is the ASCII electrode list of ANT/eemagine systems with 3-D head-frame
 * and 2-D projected positions and a unit declaration (mm / cm / m). MNE-C @c .lout
 * layouts are read by @ref mne::MNELayout.
 */

#ifndef CHANNELS_LAYOUTLOADER_H
#define CHANNELS_LAYOUTLOADER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channels_global.h"
#include <base/string_utils.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief The electrodes of an @c .elc file, in file order.
 *
 * @snippet ex_channels/main.cpp layout_loader_usage
 */
struct CHANNELSSHARED_EXPORT ElectrodeLayout
{
    mne::StringList names;                     /**< Electrode names, from the Labels block if present. */
    std::vector<Eigen::Vector3d> positions_3d; /**< 3-D positions, in @c unit. */
    std::vector<Eigen::Vector2d> positions_2d; /**< 2-D positions of the Positions2D block, if present. */
    std::string unit;                          /**< Unit of the positions, e.g. "mm". */
};

//=============================================================================================================
/**
 * Reads ANT/ASA .elc files, which hold the electrode positions of an EEG cap.
 *
 * @brief Reads ANT/ASA .elc electrode files.
 *
 * @snippet ex_channels/main.cpp layout_loader_usage
 */
class CHANNELSSHARED_EXPORT LayoutLoader
{
public:
    //=========================================================================================================
    /**
     * Reads an .elc file.
     *
     * @param[in] path  The .elc file.
     *
     * @return The electrodes, or no value if the file has no .elc extension, cannot be opened or is malformed.
     */
    static std::optional<ElectrodeLayout> read_elc(const std::filesystem::path& path);

    //=========================================================================================================
    /**
     * Parses the text of an .elc file.
     *
     * Position lines are @c "name : x y z" or plain @c "x y z"; the names of a Labels block take precedence.
     * Lines starting with @c # are comments.
     *
     * @param[in] text  The file contents.
     *
     * @return The electrodes, or no value if there are none or names and positions do not pair up.
     */
    static std::optional<ElectrodeLayout> parse_elc(std::string_view text);
};

} // namespace mne

#endif // CHANNELS_LAYOUTLOADER_H
