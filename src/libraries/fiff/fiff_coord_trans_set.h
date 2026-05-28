//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file fiff_coord_trans_set.h
 * @since February 2026
 * @brief Ordered set of FIFF coordinate transforms, the on-disk content of a FIFFB_MRI / FIFFB_HPI_MEAS block.
 *
 * Container that owns multiple @ref FiffCoordTrans instances along with the
 * auxiliary tags that travel with them inside the FIFF tree
 * (@c FIFF_MNE_RT_COMMAND comments, fiducial points from
 * @c FIFFB_ISOTRAK, ...). Used by @ref FiffStream::read_meas_info to
 * return every device→head / head→MRI / MRI→RAS transform discovered in
 * the file, and by the registration tooling
 * (@c mne_analyze, @c mne_coregistration) to load and persist coregistered
 * ``-trans.fif`` files in the same format @c mne.write_trans produces in
 * MNE-Python.
 */

#ifndef FIFFCOORDTRANSSET_H
#define FIFFCOORDTRANSSET_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class FiffCoordTrans;

//=============================================================================================================
/**
 * @brief Container for the FIFF coordinate transforms found in (or written to) a FIFFB_MRI / FIFFB_HPI_MEAS block.
 *
 * Owns the list of @ref FiffCoordTrans and the metadata that travels with
 * them (fiducial points, registration comments). Provides lookup by
 * (@c from, @c to) frame pair so consumers can ask for, e.g.,
 * @c FIFFV_COORD_HEAD → @c FIFFV_COORD_MRI without iterating manually.
 */
class FIFFSHARED_EXPORT FiffCoordTransSet
{
public:
    using SPtr = QSharedPointer<FiffCoordTransSet>;            /**< Shared pointer type for FiffCoordTransSet. */
    using ConstSPtr = QSharedPointer<const FiffCoordTransSet>; /**< Const shared pointer type for FiffCoordTransSet. */
    using UPtr = std::unique_ptr<FiffCoordTransSet>;             /**< Unique pointer type for FiffCoordTransSet. */
    using ConstUPtr = std::unique_ptr<const FiffCoordTransSet>;  /**< Const unique pointer type for FiffCoordTransSet. */

    //=========================================================================================================
    /**
     * Constructs the FiffCoordTransSet
     */
    FiffCoordTransSet();

    //=========================================================================================================
    /**
     * Destroys the FiffCoordTransSet
     */
    ~FiffCoordTransSet();

public:
    std::unique_ptr<FiffCoordTrans>    head_surf_RAS_t;   /**< Transform from MEG head coordinates to surface RAS. */
    std::unique_ptr<FiffCoordTrans>    surf_RAS_RAS_t;    /**< Transform from surface RAS to RAS (nonzero origin) coordinates. */
    std::unique_ptr<FiffCoordTrans>    RAS_MNI_tal_t;     /**< Transform from RAS (nonzero origin) to MNI Talairach coordinates. */
    std::unique_ptr<FiffCoordTrans>    MNI_tal_tal_gtz_t; /**< Transform MNI Talairach to FreeSurfer Talairach coordinates (z > 0). */
    std::unique_ptr<FiffCoordTrans>    MNI_tal_tal_ltz_t; /**< Transform MNI Talairach to FreeSurfer Talairach coordinates (z < 0). */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE FIFFLIB

#endif // FIFFCOORDTRANSSET_H
