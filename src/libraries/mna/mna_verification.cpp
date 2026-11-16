//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_verification.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaVerificationCheck, @ref MnaVerificationResult, @ref MnaProvenance and the top-level @ref MnaVerification aggregator.
 *
 * The implementation serialises each struct field-for-field and
 * delegates nested @ref MnaScript bodies and check / result lists
 * to their own codecs so a verification block round-trips
 * losslessly between JSON and CBOR. @ref MnaProvenance records its
 * software-environment strings, timing fields and input-hash map
 * verbatim so a later re-run can be compared against the original
 * execution; this is what gives MNA projects their audit-trail
 * value for clinical and publication-grade pipelines.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_verification.h"

#include "mna_codec.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// MnaVerificationCheck
//=============================================================================================================

JsonObject MnaVerificationCheck::to_json() const
{
    JsonObject json;
    json["id"] = id;
    json["description"] = description;
    json["phase"] = phase;
    json["expression"] = expression;
    if (!script.code.empty()) {
        json["script"] = script.to_json();
    }
    json["severity"] = severity;
    if (!on_fail.empty()) {
        json["on_fail"] = on_fail;
    }
    return json;
}

//=============================================================================================================

MnaVerificationCheck MnaVerificationCheck::from_json(const JsonObject& json)
{
    MnaVerificationCheck c;
    c.id = json["id"].to_std_string();
    c.description = json["description"].to_std_string();
    c.phase = json["phase"].to_std_string();
    c.expression = json["expression"].to_std_string();
    if (json.contains("script")) {
        c.script = MnaScript::from_json(json["script"].to_object());
    }
    c.severity = json["severity"].to_std_string();
    c.on_fail = json["on_fail"].to_std_string();
    return c;
}

//=============================================================================================================

CborMap MnaVerificationCheck::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaVerificationCheck MnaVerificationCheck::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}

//=============================================================================================================
// MnaVerificationResult
//=============================================================================================================

JsonObject MnaVerificationResult::to_json() const
{
    JsonObject json;
    json["check_id"] = check_id;
    json["passed"] = passed;
    json["severity"] = severity;
    json["message"] = message;
    json["actual_value"] = actual_value.is_undefined() ? JsonValue() : actual_value;
    if (evaluated_at) {
        json["evaluated_at"] = format_iso8601(*evaluated_at, true);
    }
    return json;
}

//=============================================================================================================

MnaVerificationResult MnaVerificationResult::from_json(const JsonObject& json)
{
    MnaVerificationResult r;
    r.check_id = json["check_id"].to_std_string();
    r.passed = json["passed"].to_bool(false);
    r.severity = json["severity"].to_std_string();
    r.message = json["message"].to_std_string();
    r.actual_value = json["actual_value"];
    r.evaluated_at = parse_iso8601(json["evaluated_at"].to_string());
    return r;
}

//=============================================================================================================

CborMap MnaVerificationResult::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaVerificationResult MnaVerificationResult::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}

//=============================================================================================================
// MnaProvenance
//=============================================================================================================

JsonObject MnaProvenance::to_json() const
{
    JsonObject json;
    if (!input_hashes.empty()) {
        JsonObject hashes;
        for (const auto& [port, hash] : input_hashes) {
            hashes.insert(port, hash);
        }
        json["input_hashes"] = std::move(hashes);
    }
    if (!resolved_attributes.empty())
        json["resolved_attributes"] = resolved_attributes;
    if (!mne_cpp_version.empty())
        json["mne_cpp_version"] = mne_cpp_version;
    if (!compiler_info.empty())
        json["compiler_info"] = compiler_info;
    if (!os_info.empty())
        json["os_info"] = os_info;
    if (!host_name.empty())
        json["host_name"] = host_name;
    if (!external_tool_version.empty())
        json["external_tool_version"] = external_tool_version;
    if (started_at)
        json["started_at"] = format_iso8601(*started_at, true);
    if (finished_at)
        json["finished_at"] = format_iso8601(*finished_at, true);
    if (wall_time_ms > 0)
        json["wall_time_ms"] = wall_time_ms;
    if (peak_memory_bytes > 0)
        json["peak_memory_bytes"] = peak_memory_bytes;
    if (random_seed >= 0)
        json["random_seed"] = random_seed;
    return json;
}

//=============================================================================================================

MnaProvenance MnaProvenance::from_json(const JsonObject& json)
{
    MnaProvenance p;
    for (const auto& [port, hash] : json["input_hashes"].to_object()) {
        p.input_hashes.emplace(port, hash.to_std_string());
    }
    p.resolved_attributes = json["resolved_attributes"].to_object();
    p.mne_cpp_version = json["mne_cpp_version"].to_std_string();
    p.compiler_info = json["compiler_info"].to_std_string();
    p.os_info = json["os_info"].to_std_string();
    p.host_name = json["host_name"].to_std_string();
    p.external_tool_version = json["external_tool_version"].to_std_string();
    p.started_at = parse_iso8601(json["started_at"].to_string());
    p.finished_at = parse_iso8601(json["finished_at"].to_string());
    p.wall_time_ms = static_cast<std::int64_t>(json["wall_time_ms"].to_double(0));
    p.peak_memory_bytes = static_cast<std::int64_t>(json["peak_memory_bytes"].to_double(0));
    p.random_seed = static_cast<std::int64_t>(json["random_seed"].to_double(-1));
    return p;
}

//=============================================================================================================

CborMap MnaProvenance::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaProvenance MnaProvenance::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}

//=============================================================================================================
// MnaVerification
//=============================================================================================================

JsonObject MnaVerification::to_json() const
{
    JsonObject json;
    if (!explanation.empty()) {
        json["explanation"] = explanation;
    }
    if (!checks.empty()) {
        json["checks"] = detail::to_json_array(checks);
    }
    if (!pre_results.empty()) {
        json["pre_results"] = detail::to_json_array(pre_results);
    }
    if (!post_results.empty()) {
        json["post_results"] = detail::to_json_array(post_results);
    }
    if (JsonObject provenanceJson = provenance.to_json(); !provenanceJson.empty()) {
        json["provenance"] = std::move(provenanceJson);
    }
    return json;
}

//=============================================================================================================

MnaVerification MnaVerification::from_json(const JsonObject& json)
{
    MnaVerification v;
    v.explanation = json["explanation"].to_std_string();
    v.checks = detail::from_json_array<MnaVerificationCheck>(json["checks"]);
    v.pre_results = detail::from_json_array<MnaVerificationResult>(json["pre_results"]);
    v.post_results = detail::from_json_array<MnaVerificationResult>(json["post_results"]);
    if (json.contains("provenance")) {
        v.provenance = MnaProvenance::from_json(json["provenance"].to_object());
    }
    return v;
}

//=============================================================================================================

CborMap MnaVerification::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaVerification MnaVerification::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
