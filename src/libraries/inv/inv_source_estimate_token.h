//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_source_estimate_token.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Tokeniser and de-tokeniser that turn an @ref INVLIB::InvSourceEstimate into a flat sequence consumable by foundation-model architectures.
 *
 * Declares the free functions @ref INVLIB::tokenize and
 * @ref INVLIB::fromTokens that round-trip an @ref InvSourceEstimate
 * through the @ref InvToken vocabulary defined in @c inv_token.h. The
 * tokeniser walks the estimate's metadata, dense grid, vertex list,
 * focal dipoles, coupling groups and connectivity layers and emits a
 * self-describing sequence framed by @c BOS / @c EOS markers. The
 * @ref InvTokenizeOptions struct enables sub-sampling or layer omission
 * for context-limited transformer windows, while the inverse parser
 * silently skips unknown tokens to keep the format forward-compatible.
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
