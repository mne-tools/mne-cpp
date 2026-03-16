//=============================================================================================================
/**
 * @file     inv_token.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
 * @date     June, 2026
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
 * @brief    Token vocabulary and structures for foundation-model tokenization of InvSourceEstimate.
 *
 * @details  The token vocabulary defines a self-describing sequence format for serializing
 *           multimodal neural source representations into a flat token stream that transformers
 *           and other foundation models can process.
 *
 *           Each token carries an integer vocabulary ID and an optional continuous floating-point
 *           value.  Structural tokens (section delimiters, metadata labels) use only the ID;
 *           value-carrier tokens (amplitudes, positions, times, indices) pair the ID with the
 *           continuous quantity.  This hybrid design supports both:
 *             - Models with continuous-value embedding layers (use id + value)
 *             - Purely discrete language-model architectures (use id only, apply external
 *               quantization to the value field before embedding)
 *
 *           A typical token sequence for a surface MNE estimate has the form:
 *
 *             [BOS] [META_BEGIN] [METHOD_MNE] [SPACE_SURFACE] [ORIENT_FIXED] [META_END]
 *             [GRID_BEGIN] [N_SOURCES val] [N_TIMES val] [T_MIN val] [T_STEP val]
 *               [VERTEX val] ... [VERTEX val]
 *               [GRID_ROW] [AMP val] ... [AMP val]
 *               ...
 *             [GRID_END]
 *             [EOS]
 *
 *           Dense data layers (grid amplitudes, connectivity matrices) can be optionally
 *           omitted via InvTokenizeOptions, producing a compact metadata-only sequence
 *           suitable for context-limited attention windows.
 *
 */

#ifndef INV_TOKEN_H
#define INV_TOKEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <cstdint>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Token vocabulary for the InvSourceEstimate tokenization format.
 *
 * IDs are organised into ranges:
 *   0..9     Special tokens (PAD, BOS, EOS, SEP)
 *   10..19   Metadata section markers
 *   20..29   Grid section markers
 *   30..39   Coupling section markers
 *   40..49   Focal-dipole section markers
 *   50..59   Connectivity section markers
 *   60..69   Positions section markers
 *   100..149 Method labels
 *   150..169 Source-space labels
 *   170..189 Orientation labels
 *   200..249 Value-carrier tokens (id + continuous float)
 *   250..279 Boolean tokens
 *   280..299 Dimension/size tokens (id + int-as-float)
 *   300..349 Connectivity measure name tokens
 *   1000+    Reserved for external quantisation bins
 */
enum class InvTokenId : int32_t
{
    // --- Special tokens ---
    Pad              = 0,
    Bos              = 1,       /**< Beginning of sequence. */
    Eos              = 2,       /**< End of sequence. */
    Sep              = 3,       /**< Separator (between modalities, optional). */

    // --- Section markers (paired begin / end) ---
    MetaBegin        = 10,
    MetaEnd          = 11,

    GridBegin        = 20,
    GridEnd          = 21,
    GridRow          = 22,      /**< Marks the start of one source's time-course. */

    CouplingBegin    = 30,
    CouplingEnd      = 31,
    GroupBegin        = 32,     /**< Start of one N-tuple coupling group. */
    GroupEnd          = 33,

    FocalBegin       = 40,
    FocalEnd         = 41,
    DipoleBegin      = 42,     /**< Start of one focal dipole. */
    DipoleEnd        = 43,

    ConnBegin        = 50,
    ConnEnd          = 51,
    ConnEntryBegin   = 52,     /**< Start of one connectivity measure. */
    ConnEntryEnd     = 53,

    PosBegin         = 60,
    PosEnd           = 61,

    // --- Method labels (one-hot from InvEstimateMethod) ---
    MethodUnknown    = 100,
    MethodMNE        = 101,
    MethodDSPM       = 102,
    MethodSLORETA    = 103,
    MethodELORETA    = 104,
    MethodLCMV       = 105,
    MethodDICS       = 106,
    MethodSAM        = 107,
    MethodMixedNorm  = 108,
    MethodGammaMAP   = 109,
    MethodDipoleFit  = 110,
    MethodRapMusic   = 111,
    MethodPwlRapMusic= 112,

    // --- Source-space labels (one-hot from InvSourceSpaceType) ---
    SpaceUnknown     = 150,
    SpaceSurface     = 151,
    SpaceVolume      = 152,
    SpaceMixed       = 153,
    SpaceDiscrete    = 154,

    // --- Orientation labels (one-hot from InvOrientationType) ---
    OrientUnknown    = 170,
    OrientFixed      = 171,
    OrientFree       = 172,
    OrientLoose      = 173,

    // --- Value-carrier tokens (paired with float in InvToken::value) ---
    Amplitude        = 200,     /**< Source amplitude at one (source, time) sample. */
    Vertex           = 201,     /**< Vertex / grid index. */
    TimeVal          = 202,     /**< Time in seconds. */
    FreqVal          = 203,     /**< Frequency in Hz. */
    PosX             = 204,     /**< 3-D position x component (m). */
    PosY             = 205,     /**< 3-D position y component (m). */
    PosZ             = 206,     /**< 3-D position z component (m). */
    MomX             = 207,     /**< Dipole moment x component (Am). */
    MomY             = 208,     /**< Dipole moment y component (Am). */
    MomZ             = 209,     /**< Dipole moment z component (Am). */
    Goodness         = 210,     /**< Goodness-of-fit (0..1). */
    ChiSquared       = 211,     /**< Chi-squared fit quality. */
    Correlation      = 212,     /**< Coupling correlation value. */
    ConnValue        = 213,     /**< Pairwise connectivity value. */
    GridIndex        = 214,     /**< Grid reference index inside a coupling group. */

    // --- Boolean tokens ---
    ValidTrue        = 250,
    ValidFalse       = 251,
    DirectedTrue     = 252,
    DirectedFalse    = 253,

    // --- Dimension tokens (carry size as int-in-float) ---
    NSources         = 280,     /**< Number of grid sources. */
    NTimes           = 281,     /**< Number of time samples. */
    NGroups          = 282,     /**< Number of coupling groups. */
    NDipoles         = 283,     /**< Number of focal dipoles. */
    NMeasures        = 284,     /**< Number of connectivity measures. */
    NIndices         = 285,     /**< Number of indices in a coupling group. */
    NFreeDof         = 286,     /**< Degrees of freedom. */
    TStep            = 287,     /**< Time step (s) — grid sampling interval. */

    // --- Connectivity-measure name tokens ---
    MeasCoh          = 300,
    MeasImCoh        = 301,
    MeasPlv          = 302,
    MeasPli          = 303,
    MeasWpli         = 304,
    MeasGranger      = 305,
    MeasPdc          = 306,
    MeasDtf          = 307,
    MeasCorrelation  = 308,
    MeasCrossCorr    = 309,
    MeasOther        = 310,     /**< Unrecognised metric (name not preserved in token stream). */

    // --- Quantization bin base (for fully discrete tokenisation) ---
    QuantBinBase     = 1000     /**< Bins occupy IDs [QuantBinBase .. QuantBinBase + numBins - 1]. */
};

//=============================================================================================================
/**
 * A single token in the serialised representation of an InvSourceEstimate.
 *
 * Structural tokens (section markers, metadata labels, booleans) use only the @c id field.
 * Value-carrier tokens (amplitudes, positions, times) pair @c id with a continuous @c value.
 *
 * For purely discrete architectures the continuous @c value can be quantised into a bin index
 * and encoded as @c InvTokenId::QuantBinBase + bin, discarding the float.
 *
 * @brief One element of a tokenised neural-source representation.
 */
struct InvToken
{
    InvTokenId id;      /**< Vocabulary token ID. */
    float      value;   /**< Continuous value (0 for structural tokens). */

    InvToken() : id(InvTokenId::Pad), value(0.0f) {}
    explicit InvToken(InvTokenId _id) : id(_id), value(0.0f) {}
    InvToken(InvTokenId _id, float _val) : id(_id), value(_val) {}
};

//=============================================================================================================
/**
 * Controls which layers of an InvSourceEstimate are emitted during tokenization.
 * Dense data layers (grid amplitudes, connectivity matrices) can be very large;
 * setting their flags to @c false yields a compact metadata-only token sequence
 * suitable for context-limited transformer windows.
 *
 * @brief Tokenization options controlling layer inclusion and sub-sampling.
 */
struct InvTokenizeOptions
{
    bool includeGridData     = true;   /**< Emit grid amplitude data. */
    bool includeCouplings    = true;   /**< Emit source coupling annotations. */
    bool includeFocalDipoles = true;   /**< Emit focal dipole layer. */
    bool includeConnectivity = true;   /**< Emit connectivity matrices. */
    bool includePositions    = true;   /**< Emit explicit 3-D positions. */
    int  maxSources          = -1;     /**< Sub-sample grid sources (-1 = all). */
    int  maxTimePoints       = -1;     /**< Sub-sample time points  (-1 = all). */
};

//=============================================================================================================
/**
 * Extract just the integer token IDs from a token sequence (discarding continuous values).
 * Useful for feeding into a purely discrete embedding table.
 *
 * @param[in] tokens    The token sequence.
 * @return Vector of int32_t token IDs.
 */
inline std::vector<int32_t> tokenIds(const std::vector<InvToken>& tokens)
{
    std::vector<int32_t> ids(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i)
        ids[i] = static_cast<int32_t>(tokens[i].id);
    return ids;
}

//=============================================================================================================
/**
 * Extract just the continuous values from a token sequence (discarding IDs).
 * Useful as the real-valued branch in a hybrid discrete-continuous embedding.
 *
 * @param[in] tokens    The token sequence.
 * @return Vector of float values.
 */
inline std::vector<float> tokenValues(const std::vector<InvToken>& tokens)
{
    std::vector<float> vals(tokens.size());
    for (size_t i = 0; i < tokens.size(); ++i)
        vals[i] = tokens[i].value;
    return vals;
}

} // NAMESPACE INVLIB

#endif // INV_TOKEN_H
