//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_types.h
 * @since March 2026
 * @brief Strongly-typed enumerations that tag a source estimate with the inverse method, source-space type, and orientation constraint that produced it.
 *
 * Defines @ref INVLIB::InvEstimateMethod (MNE, dSPM, sLORETA, eLORETA,
 * LCMV, DICS, SAM, MixedNorm, GammaMAP, DipoleFit, RapMusic, PwlRapMusic),
 * @ref INVLIB::InvSourceSpaceType (surface, volume, mixed, discrete) and
 * @ref INVLIB::InvOrientationType (fixed, free, loose). These tags travel
 * on every @ref INVLIB::InvSourceEstimate so downstream consumers
 * (visualisation, ROI extraction, tokenisation, CSV export) can branch on
 * the algorithm and geometry that produced the data without re-inspecting
 * matrix shapes.
 */

#ifndef INV_TYPES_H
#define INV_TYPES_H

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Enumerates the inverse estimation method that produced a source estimate.
 */
enum class InvEstimateMethod
{
    Unknown = 0,
    MNE,
    dSPM,
    sLORETA,
    eLORETA,
    LCMV,
    DICS,
    SAM,
    MixedNorm,
    GammaMAP,
    DipoleFit,
    RapMusic,
    PwlRapMusic
};

//=============================================================================================================
/**
 * Enumerates the source space type underlying a source estimate.
 */
enum class InvSourceSpaceType
{
    Unknown = 0,
    Surface,
    Volume,
    Mixed,
    Discrete
};

//=============================================================================================================
/**
 * Enumerates the orientation constraint used during source estimation.
 */
enum class InvOrientationType
{
    Unknown = 0,
    Fixed,
    Free,
    Loose
};

} // NAMESPACE INVLIB

#endif // INV_TYPES_H
