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
 * @brief    Tokenization and de-tokenization functions for InvSourceEstimate.
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
 * Serialise an InvSourceEstimate into a flat token sequence for foundation-model consumption.
 * Each token carries a vocabulary ID and an optional continuous value.
 * The options parameter controls which layers are emitted and allows sub-sampling
 * of dense data to fit context-limited attention windows.
 *
 * @param[in] estimate   The source estimate to tokenize.
 * @param[in] options    Controls layer inclusion and sub-sampling.
 * @return A vector of InvToken representing the serialised estimate.
 */
INVSHARED_EXPORT std::vector<InvToken> tokenize(const InvSourceEstimate& estimate,
                                                const InvTokenizeOptions& options = InvTokenizeOptions());

//=============================================================================================================
/**
 * Reconstruct an InvSourceEstimate from a token sequence previously produced by tokenize().
 *
 * @param[in] tokens    The token sequence.
 * @return The reconstructed source estimate.
 */
INVSHARED_EXPORT InvSourceEstimate fromTokens(const std::vector<InvToken>& tokens);

} // NAMESPACE INVLIB

#endif // INV_SOURCE_ESTIMATE_TOKEN_H
