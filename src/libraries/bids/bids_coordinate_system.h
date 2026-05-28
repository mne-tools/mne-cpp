//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_coordinate_system.h
 * @since March 2026
 * @brief Reader/writer for the BIDS ``_coordsystem.json`` sidecar describing the spatial reference frame of an ``_electrodes.tsv`` record.
 *
 * @c _coordsystem.json is REQUIRED for every iEEG (and recommended for
 * EEG) electrode-position file and pins down three things: the name of
 * the coordinate system (@c ACPC, @c MNI305, @c CapTrak, @c Other, …),
 * the unit of the @c x/@c y/@c z columns in the matching
 * @c _electrodes.tsv (@c m, @c mm or @c cm) and an optional 4×4 affine
 * linking that frame to an associated anatomical image. @ref
 * BidsCoordinateSystem is the value object that captures those fields
 * plus the recommended free-text description / processing-description /
 * @c IntendedFor pointer.
 *
 * @ref BidsCoordinateSystem::toFiffCoordTrans bridges the parsed affine
 * into a @c FIFFLIB::FiffCoordTrans so downstream MNE-CPP code (forward
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
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Coordinate system metadata from *_coordsystem.json.
 *
 * Describes the spatial reference frame used for electrode positions.
 */
struct BIDSSHARED_EXPORT BidsCoordinateSystem
{
    QString system;                     /**< e.g. "ACPC", "MNI305", "Other" (REQUIRED for iEEG). */
    QString units;                      /**< "m", "mm", or "cm" (REQUIRED for iEEG). */
    QString description;                /**< Description of the coordinate system (RECOMMENDED). */
    QString processingDescription;      /**< How coordinates were obtained (RECOMMENDED). */
    QString associatedImagePath;        /**< Relative path to associated T1w image (OPTIONAL). */
    Eigen::Matrix4d transform;          /**< 4x4 affine transform (identity if not provided). */

    /**
     * @brief Read a BIDS *_coordsystem.json file.
     * @param[in] sFilePath  Path to the coordsystem.json file.
     * @return Populated coordinate system, or default if file cannot be read.
     */
    static BidsCoordinateSystem readJson(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_coordsystem.json file.
     * @param[in] sFilePath  Output path.
     * @param[in] cs         Coordinate system metadata.
     * @return true on success.
     */
    static bool writeJson(const QString& sFilePath,
                          const BidsCoordinateSystem& cs);

    /**
     * @brief Convert parsed transform to a FiffCoordTrans.
     * @param[in] fromFrame  Source coordinate frame (default FIFFV_COORD_MRI = 5).
     * @param[in] toFrame    Destination coordinate frame (default FIFFV_COORD_HEAD = 4).
     * @return FiffCoordTrans populated with the parsed 4x4 matrix.
     */
    FIFFLIB::FiffCoordTrans toFiffCoordTrans(int fromFrame = FIFFV_COORD_MRI,
                                              int toFrame = FIFFV_COORD_HEAD) const;
};

} // namespace BIDSLIB

#endif // BIDS_COORDINATE_SYSTEM_H
