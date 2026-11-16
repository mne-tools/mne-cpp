//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_param_binding.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Formula-driven binding that recomputes one MNA parameter from an expression whenever its triggers fire.
 *
 * @ref mne::MnaParamBinding turns the otherwise static @ref mne::MnaParamTree
 * into a reactive system. Each binding links a @c target_path
 * (typically @c nodeId/attrKey) to an @c expression that references
 * upstream results or other parameters through the @c ref("…")
 * helper — for example @c "clamp(ref('noise_est_01/snr') * 0.1,
 * @c 0.01, @c 1.0)" — so a downstream node's regularisation can be
 * derived live from an SNR estimate produced earlier in the graph.
 *
 * The @c trigger field selects when the formula is re-evaluated:
 * @c on_change fires whenever any path in @c dependencies updates,
 * @c periodic re-evaluates every @c period_ms (useful for streaming
 * pipelines), and @c manual leaves evaluation to explicit user
 * action. JSON/CBOR round-trip is symmetric so bindings survive
 * project save/load without loss of semantics.
 */

#ifndef MNA_PARAM_BINDING_H
#define MNA_PARAM_BINDING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"

#include <base/cbor.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <string>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Formula that recomputes one parameter of an @ref MnaParamTree.
 *
 * @brief Dynamic parameter binding for the MNA parameter tree.
 *
 * @snippet ex_mna/main.cpp mna_param_tree_binding
 */
struct MNASHARED_EXPORT MnaParamBinding
{
    std::string target_path; ///< Parameter to control: "nodeId/attrKey"
    std::string expression;  ///< Formula string, e.g. "clamp(ref('noise_est_01/snr') * 0.1, 0.01, 1.0)"
    std::string trigger;     ///< "on_change", "periodic", "manual"
    int period_ms = 0;       ///< Evaluation period when trigger == "periodic" (ignored otherwise)
    StringList dependencies; ///< Paths this binding reads from

    /** @return The binding as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A binding as written by to_json(). @return The binding. */
    static MnaParamBinding from_json(const JsonObject& json);

    /** @return The binding as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A binding as written by to_cbor(). @return The binding. */
    static MnaParamBinding from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_PARAM_BINDING_H
