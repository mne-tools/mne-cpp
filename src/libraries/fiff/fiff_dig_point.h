//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_dig_point.h
 * @since 2022
 * @date  March 2026
 * @brief Single digitization point (FIFF_DIG_POINT) with kind (cardinal/HPI/EEG/extra), identifier and 3D coordinates.
 *
 * The Polhemus / optical digitizer used for MEG/EEG coregistration writes
 * one @c FIFF_DIG_POINT tag per landmark into the @c FIFFB_ISOTRAK block.
 * Each tag carries a kind (@c FIFFV_POINT_CARDINAL for nasion / LPA / RPA,
 * @c FIFFV_POINT_HPI for head-position-indicator coils, @c FIFFV_POINT_EEG
 * for electrode positions, @c FIFFV_POINT_EXTRA for additional head-shape
 * samples), a per-kind index, and the (x, y, z) position in metres in the
 * @c FIFFV_COORD_HEAD frame.
 *
 * @ref FiffDigPoint wraps that record. The point cloud assembled from
 * all dig points drives head-shape based coregistration and the iterative
 * closest-point fit in @c mne_analyze, mirroring the
 * @c info['dig'] list consumed by @c mne.io.Info in MNE-Python.
 */

#ifndef FIFF_DIG_POINT_H
#define FIFF_DIG_POINT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QDebug>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief One digitizer point: kind (cardinal/HPI/EEG/extra), ident, 3D position in @c FIFFV_COORD_HEAD.
 *
 * Field layout matches @c fiffDigPointRec on disk. The (@c kind, @c ident)
 * pair is the lookup key used everywhere else in the codebase — for
 * example, the LPA/Nasion/RPA fiducials are found by searching for
 * @c kind == @c FIFFV_POINT_CARDINAL and @c ident == 1 / 2 / 3.
 */
class FIFFSHARED_EXPORT FiffDigPoint
{
public:
    using SPtr = QSharedPointer<FiffDigPoint>;            /**< Shared pointer type for FiffDigPoint. */
    using ConstSPtr = QSharedPointer<const FiffDigPoint>; /**< Const shared pointer type for FiffDigPoint. */
    using UPtr = std::unique_ptr<FiffDigPoint>;             /**< Unique pointer type for FiffDigPoint. */
    using ConstUPtr = std::unique_ptr<const FiffDigPoint>;  /**< Const unique pointer type for FiffDigPoint. */

    //=========================================================================================================
    /**
     * Constructs the digitization point description
     */
    FiffDigPoint();

    //=========================================================================================================
    /**
     * Destroys the digitization point description
     */
    ~FiffDigPoint() = default;

    //=========================================================================================================
    /**
     * Size of the old struct (fiffDigPointRec) 5*int = 5*4 = 20
     *
     * @return the size of the old struct fiffDigPointRec.
     */
    inline static qint32 storageSize();

public:
    fiff_int_t      kind;           /**< FIFFV_POINT_CARDINAL, FIFFV_POINT_HPI, FIFFV_POINT_EXTRA or FIFFV_POINT_EEG. */
    fiff_int_t      ident;          /**< Number identifying this point. */
    fiff_float_t    r[3];           /**< Point location. */
    fiff_int_t      coord_frame;    /**< Newly added to stay consistent with fiff MATLAB implementation. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffDigPoint::storageSize()
{
    return sizeof(FiffDigPoint::kind) + sizeof(FiffDigPoint::ident)
         + sizeof(FiffDigPoint::r);  // coord_frame is not part of on-disk format
}
} // NAMESPACE

#endif // FIFF_DIG_POINT_H
