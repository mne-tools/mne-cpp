//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_file_ref.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaFileRef, including base64 framing of embedded payloads and preservation of unknown keys in @c extras.
 *
 * The implementation is symmetric: every field declared in the
 * struct round-trips losslessly through both the JSON and CBOR
 * paths, embedded bytes are encoded as base64 in JSON (for
 * human-editability) and as a native byte string in CBOR (for
 * compactness), and any key not recognised by the current build
 * is shovelled into @c extras on read and re-emitted verbatim on
 * write to keep forward compatibility intact.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_file_ref.h"

#include "mna_codec.h"

#include <mne/base/byte_stream.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** @return The JSON form without the embedded data. */
JsonObject metadata(const MnaFileRef& ref)
{
    JsonObject json = ref.extras;
    json["role"] = to_string(ref.role);
    json["path"] = ref.path;
    json["sha256"] = ref.sha256;
    json["format"] = ref.format;
    json["size_bytes"] = ref.size_bytes;
    json["embedded"] = ref.embedded;
    return json;
}

/** @return The reference described by @p json, without the embedded data. */
MnaFileRef from_metadata(const JsonObject& json)
{
    MnaFileRef ref;
    ref.role = file_role_from_string(json["role"].to_string());
    ref.path = json["path"].to_std_string();
    ref.sha256 = json["sha256"].to_std_string();
    ref.format = json["format"].to_std_string();
    ref.size_bytes = static_cast<std::int64_t>(json["size_bytes"].to_double());
    ref.embedded = json["embedded"].to_bool();
    ref.extras = json.without({"role", "path", "sha256", "format", "size_bytes", "embedded", "data"});
    return ref;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaFileRef::to_json() const
{
    JsonObject json = metadata(*this);
    if (embedded && !data.empty()) {
        json["data"] = to_base64(data);
    }
    return json;
}

//=============================================================================================================

MnaFileRef MnaFileRef::from_json(const JsonObject& json)
{
    MnaFileRef ref = from_metadata(json);
    if (ref.embedded) {
        ref.data = from_base64(json["data"].to_string()).value_or(std::vector<std::byte>{});
    }
    return ref;
}

//=============================================================================================================

CborMap MnaFileRef::to_cbor() const
{
    CborMap cbor = CborMap::from_json_object(metadata(*this));
    if (embedded && !data.empty()) {
        cbor.insert("data", data);
    }
    return cbor;
}

//=============================================================================================================

MnaFileRef MnaFileRef::from_cbor(const CborMap& cbor)
{
    MnaFileRef ref = from_metadata(cbor.to_json_object({"data"})); // the payload stays binary
    if (ref.embedded) {
        ref.data = cbor["data"].to_byte_array();
    }
    return ref;
}

//=============================================================================================================

Expected<void> MnaFileRef::embed(const std::filesystem::path& file)
{
    Expected<std::vector<std::byte>> bytes = read_file_bytes(file);
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    data = std::move(*bytes);
    size_bytes = static_cast<std::int64_t>(data.size());
    embedded = true;
    return {};
}

//=============================================================================================================

Expected<void> MnaFileRef::extract(const std::filesystem::path& file)
{
    if (Expected<void> written = write_file_bytes(file, data); !written) {
        return written;
    }
    data.clear();
    embedded = false;
    return {};
}
