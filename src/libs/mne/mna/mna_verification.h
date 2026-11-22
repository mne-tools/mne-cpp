//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_verification.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Verification, declarative checks, evaluation results and provenance snapshot attached to every @ref mne::MnaNode for reproducible execution.
 *
 * This header carries the four structs that turn an MNA graph from
 * a black-box runner into a self-auditing pipeline. @ref mne::MnaVerificationCheck
 * is a declarative pre- or post-condition (e.g. @c rank(covariance)
 * @c > @c 0) authored alongside the node; @ref mne::MnaVerificationResult
 * records each evaluation outcome with the actual value, severity
 * and timestamp; and the parent @ref mne::MnaVerification aggregates an
 * explanation string, the check list, both result lists and the
 * full @ref mne::MnaProvenance snapshot.
 *
 * @ref mne::MnaProvenance captures everything needed to re-run a node
 * bit-for-bit later: SHA-256 of every input, the resolved parameter
 * map after @ref mne::MnaParamTree evaluation, MNE-CPP / Qt / compiler /
 * OS versions, external tool versions for IPC and Script nodes,
 * wall-clock and peak-RSS measurements, and the random seed when
 * stochastic ops are involved. Together these structs are what
 * lets MNA projects double as audit trails for clinical and
 * publication-grade analyses.
 */

#ifndef MNA_VERIFICATION_H
#define MNA_VERIFICATION_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_script.h"

#include <mne/base/cbor.h>
#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

/**
 * A declarative check (pre- or post-condition) attached to a graph node.
 * Evaluated by the executor; can warn, abort, or log an informational note.
 *
 * @brief Verification check for a graph node.
 *
 * @snippet ex_mna/main.cpp mna_verification_usage
 */
struct MNASHARED_EXPORT MnaVerificationCheck
{
    std::string id;          ///< Unique check identifier within the node (e.g. "cov_posdef")
    std::string description; ///< Human-readable: "Covariance matrix must be positive-definite"
    std::string phase;       ///< "pre" (before execution) or "post" (after execution)
    std::string expression;  ///< MnaParamTree expression over attributes, inputs and (post) outputs: "rank > 0"
    MnaScript script;        ///< Optional script for complex checks (exit code 0 = pass).
                             ///< When script.code is non-empty, the executor runs the script
                             ///< instead of evaluating `expression`. Supports {{placeholder}}
                             ///< substitution for node inputs and attributes.
    std::string severity;    ///< "error" (abort), "warning" (log + continue), "info" (always continue)
    std::string on_fail;     ///< Optional remediation hint

    /** @return The check as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A check as written by to_json(). @return The check. */
    static MnaVerificationCheck from_json(const JsonObject& json);

    /** @return The check as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A check as written by to_cbor(). @return The check. */
    static MnaVerificationCheck from_cbor(const CborMap& cbor);
};

/**
 * The result of evaluating a single verification check.
 *
 * @brief Check evaluation result.
 *
 * @snippet ex_mna/main.cpp mna_verification_usage
 */
struct MNASHARED_EXPORT MnaVerificationResult
{
    std::string check_id;                ///< References MnaVerificationCheck::id
    bool passed = false;                 ///< true if the expression evaluated to true
    std::string severity;                ///< Echoed from the check definition
    std::string message;                 ///< Formatted message: "PASS: ..." or "FAIL [error]: ..."
    JsonValue actual_value;              ///< The evaluated expression result
    std::optional<UtcTime> evaluated_at; ///< When this check was evaluated

    /** @return The result as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A result as written by to_json(). @return The result. */
    static MnaVerificationResult from_json(const JsonObject& json);

    /** @return The result as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A result as written by to_cbor(). @return The result. */
    static MnaVerificationResult from_cbor(const CborMap& cbor);
};

/**
 * Complete provenance snapshot captured after node execution.
 * Records input hashes, resolved parameter values, software versions, and timing.
 *
 * @brief Provenance record for reproducibility.
 *
 * @snippet ex_mna/main.cpp mna_verification_usage
 */
struct MNASHARED_EXPORT MnaProvenance
{
    std::map<std::string, std::string> input_hashes; ///< portName → SHA-256
    JsonObject resolved_attributes;                  ///< Attributes at execution time (after param-tree evaluation)

    std::string mne_cpp_version; ///< e.g. "2.2.0"
    std::string compiler_info;   ///< e.g. "AppleClang 16.0.0"
    std::string os_info;         ///< e.g. "macOS 15.4 arm64"
    std::string host_name;       ///< Machine name (for cluster provenance)

    std::string external_tool_version; ///< For IPC/Script nodes, e.g. "FreeSurfer 7.4.1", "Python 3.11.5"

    std::optional<UtcTime> started_at;  ///< When execution started
    std::optional<UtcTime> finished_at; ///< When execution finished
    std::int64_t wall_time_ms = 0;      ///< Wall-clock duration in milliseconds
    std::int64_t peak_memory_bytes = 0; ///< Peak RSS (if measurable), 0 otherwise

    std::int64_t random_seed = -1; ///< Seed of stochastic operations; -1 if not applicable

    /** @return The provenance as a JSON object. */
    JsonObject to_json() const;

    /** @param[in] json   A provenance record as written by to_json(). @return The provenance. */
    static MnaProvenance from_json(const JsonObject& json);

    /** @return The provenance as a CBOR map. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A provenance record as written by to_cbor(). @return The provenance. */
    static MnaProvenance from_cbor(const CborMap& cbor);
};

/**
 * Top-level verification container attached to each MnaNode.
 * Combines human-readable explanation, declarative checks, evaluation results,
 * and a complete provenance snapshot.
 *
 * @brief Verification, explanation, and provenance for a graph node.
 *
 * @snippet ex_mna/main.cpp mna_verification_usage
 */
struct MNASHARED_EXPORT MnaVerification
{
    std::string explanation;                         ///< What this node does and why
    std::vector<MnaVerificationCheck> checks;        ///< Declarative checks (authored by user, evaluated by executor)
    std::vector<MnaVerificationResult> pre_results;  ///< Results of pre-execution checks (populated by executor)
    std::vector<MnaVerificationResult> post_results; ///< Results of post-execution checks (populated by executor)
    MnaProvenance provenance;                        ///< Complete provenance snapshot (populated by executor)

    /** @return The verification as a JSON object, empty if nothing is set. */
    JsonObject to_json() const;

    /** @param[in] json   A verification as written by to_json(). @return The verification. */
    static MnaVerification from_json(const JsonObject& json);

    /** @return The verification as a CBOR map, empty if nothing is set. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A verification as written by to_cbor(). @return The verification. */
    static MnaVerification from_cbor(const CborMap& cbor);
};

} // namespace mne

#endif // MNA_VERIFICATION_H
