//=============================================================================================================
/**
 * @file     inv_source_estimate_token.h
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.0.0
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
 * @brief    Tokenization and de-tokenization of InvSourceEstimate for foundation-model interfacing.
 *
 * @details  Provides free functions to serialise an InvSourceEstimate into a flat
 *           token sequence (see inv_token.h for the vocabulary) and to
 *           reconstruct an estimate from such a sequence.  These are the public
 *           entry points; all vocabulary IDs, the InvToken struct, and the
 *           InvTokenizeOptions knobs live in inv_token.h.
 *
 */

#ifndef INV_SOURCE_ESTIMATE_TOKEN_H
#define INV_SOURCE_ESTIMATE_TOKEN_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"
#include "inv_token.h"

#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace INVLIB
{
class InvSourceEstimate;
}

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Serialise an InvSourceEstimate into a flat token sequence.
 *
 * Converts the full multimodal content of an InvSourceEstimate (grid amplitudes,
 * positions, couplings, focal dipoles, connectivity matrices) into a linear
 * sequence of InvToken elements.  Each token carries a vocabulary ID and an
 * optional continuous float value.
 *
 * The @p options parameter controls which data layers are emitted and allows
 * sub-sampling of dense grids to fit context-limited attention windows.
 *
 * @param[in] estimate   The source estimate to tokenize.
 * @param[in] options    Controls layer inclusion and sub-sampling (default: all layers, no sub-sampling).
 * @return A vector of InvToken representing the serialised estimate.
 *
 * @sa fromTokens, InvTokenizeOptions, InvToken
 */
INVSHARED_EXPORT std::vector<InvToken> tokenize(const InvSourceEstimate& estimate,
                                                const InvTokenizeOptions& options = InvTokenizeOptions());

//=============================================================================================================
/**
 * @brief Reconstruct an InvSourceEstimate from a token sequence.
 *
 * Parses a token stream previously produced by tokenize() and rebuilds the
 * InvSourceEstimate fields (metadata, grid data, positions, couplings,
 * focal dipoles, connectivity).  Unknown or out-of-order tokens are
 * silently skipped.
 *
 * @param[in] tokens    The token sequence to decode.
 * @return The reconstructed source estimate.
 *
 * @sa tokenize, InvToken
 */
INVSHARED_EXPORT InvSourceEstimate fromTokens(const std::vector<InvToken>& tokens);

} // NAMESPACE INVLIB

#endif // INV_SOURCE_ESTIMATE_TOKEN_H
