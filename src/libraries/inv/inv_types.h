//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_types.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Strongly-typed enumerations that tag a source estimate with the inverse method, source-space type, and orientation constraint that produced it.
 *
 * Defines @ref mne::InvEstimateMethod (MNE, dSPM, sLORETA, eLORETA,
 * LCMV, DICS, SAM, MixedNorm, GammaMAP, DipoleFit, RapMusic, PwlRapMusic),
 * @ref mne::InvSourceSpaceType (surface, volume, mixed, discrete) and
 * @ref mne::InvOrientationType (fixed, free, loose). These tags travel
 * on every @ref mne::InvSourceEstimate so downstream consumers
 * (visualisation, ROI extraction, tokenisation, CSV export) can branch on
 * the algorithm and geometry that produced the data without re-inspecting
 * matrix shapes.
 */

#ifndef INV_TYPES_H
#define INV_TYPES_H

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
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

} // NAMESPACE mne_inv

#endif // INV_TYPES_H
