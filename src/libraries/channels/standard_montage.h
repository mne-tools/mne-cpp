//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     standard_montage.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Standard 10-20 / 10-10 / 10-05 EEG electrode positions of MNE-Python's standard_1005 montage.
 *
 * @ref mne::StandardMontage gives every consumer in mne-cpp (the coregistration wizard,
 * the layout maker, forward modelling) a default cap when no digitised positions exist. The
 * positions are those of mne.channels.make_standard_montage("standard_1005"), in metres in
 * the MNI (fsaverage MRI) frame; the 10-20 and 10-10 systems are its electrodes of those names.
 */

#ifndef CHANNELS_STANDARD_MONTAGE_H
#define CHANNELS_STANDARD_MONTAGE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "channels_global.h"

#include <base/string_utils.h>

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Represents an electrode position in a montage.
 *
 * @snippet ex_channels/main.cpp standard_montage_usage
 */
struct CHANNELSSHARED_EXPORT ElectrodePosition
{
    std::string name;    /**< Electrode name (e.g. "Cz", "Fp1"). */
    Eigen::Vector3d pos; /**< Position in metres, MNI (fsaverage MRI) coordinates. */
};

//=============================================================================================================
/**
 * @brief Standard EEG montage with named electrode positions.
 *
 * @snippet ex_channels/main.cpp standard_montage_usage
 */
class CHANNELSSHARED_EXPORT StandardMontage
{
public:
    //=========================================================================================================
    /**
     * @brief Supported standard montage systems.
     */
    enum class System
    {
        Standard_1020, /**< 10-20 system (21 electrodes, including A1/A2). */
        Standard_1010, /**< 10-10 system (73 positions, including the 10-20 ones and Nz). */
        Standard_1005  /**< 10-05 system (343 electrodes, mne's standard_1005 montage). */
    };

    //=========================================================================================================
    /**
     * @brief The electrodes of a montage system.
     *
     * @param[in] system  The montage system.
     *
     * @return The electrode positions.
     */
    static std::vector<ElectrodePosition> montage(System system);

    //=========================================================================================================
    /**
     * @brief The electrode names of a montage system.
     *
     * @param[in] system  The montage system.
     *
     * @return The electrode names, in montage order.
     */
    static mne::StringList electrode_names(System system);

    //=========================================================================================================
    /**
     * @brief Looks up an electrode of the standard_1005 table, fiducials included.
     *
     * @param[in] name  Electrode name, case-insensitive.
     *
     * @return The position, or no value if the name is unknown.
     */
    static std::optional<Eigen::Vector3d> find_electrode(std::string_view name);

    //=========================================================================================================
    /**
     * @brief The number of electrodes of a montage system.
     *
     * @param[in] system  The montage system.
     *
     * @return The electrode count.
     */
    static std::size_t electrode_count(System system);
};

} // namespace mne

#endif // CHANNELS_STANDARD_MONTAGE_H
