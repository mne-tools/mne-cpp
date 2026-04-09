//=============================================================================================================
/**
 * @file     inv_beamformer_settings.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    Beamformer settings and enumerations.
 *
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
