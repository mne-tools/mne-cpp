//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_beamformer_settings.h
 * @since March 2026
 * @brief Strongly-typed enumerations shared by the LCMV and DICS beamformer pipelines.
 *
 * Defines @ref INVLIB::BeamformerWeightNorm (none / unit-noise-gain /
 * NAI / rotation-invariant unit-noise-gain), @ref BeamformerPickOri
 * (keep-all / surface-normal / max-power / 3-vector) and
 * @ref BeamformerInversion (full-matrix vs. scalar denominator
 * inversion). These knobs are the user-facing API surface that
 * @ref InvLCMV / @ref InvDICS expose to control how the spatial-filter
 * weights are derived from the leadfield and data covariance / CSD.
 * The normalisation choices follow Sekihara &amp; Nagarajan,
 * @em Adaptive Spatial Filters for Electromagnetic Brain Imaging,
 * Springer (2008).
 */

#ifndef INV_BEAMFORMER_SETTINGS_H
#define INV_BEAMFORMER_SETTINGS_H

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Weight normalization strategy for beamformer spatial filters.
 *
 * References:
 * - Sekihara & Nagarajan, "Adaptive Spatial Filters for Electromagnetic Brain Imaging", 2008.
 * - Van Veen et al., IEEE Trans. Biomed. Eng. 44(9), 867-880, 1997.
 */
enum class BeamformerWeightNorm
{
    None = 0,           /**< No normalization (raw unit-gain filter). */
    UnitNoiseGain,      /**< Normalize by sqrt(diag(W @ W^H)), Sekihara 2008. */
    NAI,                /**< Neural Activity Index: unit-noise-gain + noise subspace normalization. */
    UnitNoiseGainInv    /**< Rotation-invariant unit-noise-gain via sqrtm(inner)^{-0.5}. */
};

//=============================================================================================================
/**
 * Orientation picking mode for beamformer filters.
 */
enum class BeamformerPickOri
{
    None = 0,           /**< Keep all orientations (3 per source for free, 1 for fixed). */
    Normal,             /**< Extract surface normal component only (Z in local coords). */
    MaxPower,           /**< Optimal orientation via max eigenvalue of power matrix. */
    Vector              /**< Return the full 3-component vector solution. */
};

//=============================================================================================================
/**
 * Method for inverting the beamformer denominator matrix G^H Cm^{-1} G.
 */
enum class BeamformerInversion
{
    Matrix = 0,         /**< Full matrix inversion per source. */
    Single              /**< Scalar (diagonal) inversion per source. */
};

} // NAMESPACE INVLIB

#endif // INV_BEAMFORMER_SETTINGS_H
