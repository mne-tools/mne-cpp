//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_script.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaScript including code body, interpreter selection, integrity hash and optional source URI.
 *
 * The implementation writes the script @c code verbatim (after
 * resolving an optional @c source_uri to the on-disk file at save
 * time), records the SHA-256 in @c code_sha256, and serialises the
 * interpreter command plus @c interpreter_args in execution order.
 * The @c keep_temp_file debug flag and the @c language /
 * @c interpreter pair round-trip symmetrically so a project can be
 * re-executed bit-for-bit later. Unknown fields are captured into
 * the standard @c extras pattern used across mne_mna structs.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_script.h"

#include "mna_codec.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaScript::to_json() const
{
    JsonObject json;
    json["language"] = language;
    if (!interpreter.empty()) {
        json["interpreter"] = interpreter;
    }
    if (!interpreter_args.empty()) {
        json["interpreter_args"] = JsonArray::from_string_list(interpreter_args);
    }
    json["code"] = code;
    if (!source_uri.empty()) {
        json["source_uri"] = source_uri;
    }
    if (!code_sha256.empty()) {
        json["code_sha256"] = code_sha256;
    }
    if (keep_temp_file) {
        json["keep_temp_file"] = true;
    }
    return json;
}

//=============================================================================================================

MnaScript MnaScript::from_json(const JsonObject& json)
{
    MnaScript s;
    s.language = json["language"].to_std_string();
    s.interpreter = json["interpreter"].to_std_string();
    s.interpreter_args = json["interpreter_args"].to_array().to_string_list();
    s.code = json["code"].to_std_string();
    s.source_uri = json["source_uri"].to_std_string();
    s.code_sha256 = json["code_sha256"].to_std_string();
    s.keep_temp_file = json["keep_temp_file"].to_bool(false);
    return s;
}

//=============================================================================================================

CborMap MnaScript::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaScript MnaScript::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
