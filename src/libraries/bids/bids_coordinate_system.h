//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_coordinate_system.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Reader/writer for the BIDS ``_coordsystem.json`` sidecar describing the spatial reference frame of an ``_electrodes.tsv`` record.
 *
 * @c _coordsystem.json is REQUIRED for every iEEG (and recommended for
 * EEG) electrode-position file and pins down three things: the name of
 * the coordinate system (@c ACPC, @c MNI305, @c CapTrak, @c Other, …),
 * the unit of the @c x/@c y/@c z columns in the matching
 * @c _electrodes.tsv (@c m, @c mm or @c cm) and an optional 4×4 affine
 * linking that frame to an associated anatomical image. @ref
 * mne::BidsCoordinateSystem is the value object that captures those fields
 * plus the recommended free-text description / processing-description /
 * @c IntendedFor pointer.
 *
 * @ref mne::BidsCoordinateSystem::to_fiff_coord_trans "BidsCoordinateSystem::to_fiff_coord_trans" bridges the parsed affine
 * into a @c mne::FiffCoordTrans so downstream MNE-CPP code (forward
 * solution, source localisation, visualisation) can treat a BIDS
 * dataset's coordinate metadata exactly like a FIFF MRI↔head transform.
 * The BIDS↔FIFF coordinate-frame name mapping lives in @ref
 * bids_const.h.
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/modality-specific-files/electrophysiology.html#coordinate-system-json
 */

#ifndef BIDS_COORDINATE_SYSTEM_H
#define BIDS_COORDINATE_SYSTEM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

#include <fiff/fiff_coord_trans.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <string_view>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Coordinate system metadata from *_coordsystem.json.
 *
 * Describes the spatial reference frame used for electrode positions.
 *
 * @snippet ex_bids/main.cpp bids_sidecar_records
 */
struct BIDSSHARED_EXPORT BidsCoordinateSystem
{
    std::string system;                /**< e.g. "ACPC", "MNI305", "Other" (REQUIRED for iEEG). */
    std::string units;                 /**< "m", "mm", or "cm" (REQUIRED for iEEG). */
    std::string description;           /**< Description of the coordinate system (RECOMMENDED). */
    std::string processingDescription; /**< How coordinates were obtained (RECOMMENDED). */
    std::string associatedImagePath;   /**< Relative path to associated T1w image (OPTIONAL). */
    Eigen::Matrix4d transform;         /**< 4x4 affine transform (identity if not provided). */

    /**
     * @brief Read a BIDS *_coordsystem.json file.
     * @param[in] path       Path to the coordsystem.json file.
     * @return Populated coordinate system, or default if file cannot be read.
     */
    static BidsCoordinateSystem read_json(const std::filesystem::path& path);

    /**
     * @brief Write a BIDS *_coordsystem.json file.
     * @param[in] path       Output path.
     * @param[in] cs         Coordinate system metadata.
     * @param[in] sModality  Key prefix of the datatype: "iEEG", "EEG" or "MEG".
     * @return true on success.
     */
    static bool write_json(const std::filesystem::path& path,
                           const BidsCoordinateSystem& cs,
                           std::string_view sModality = "iEEG");

    /**
     * @brief Convert parsed transform to a FiffCoordTrans.
     * @param[in] fromFrame  Source coordinate frame (default FIFFV_COORD_MRI = 5).
     * @param[in] toFrame    Destination coordinate frame (default FIFFV_COORD_HEAD = 4).
     * @return FiffCoordTrans populated with the parsed 4x4 matrix.
     */
    mne::FiffCoordTrans to_fiff_coord_trans(int fromFrame = FIFFV_COORD_MRI,
                                            int toFrame = FIFFV_COORD_HEAD) const;
};

} // namespace mne

#endif // BIDS_COORDINATE_SYSTEM_H
