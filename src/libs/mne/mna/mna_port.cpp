//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_port.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaPort including data-kind enum mapping, optional stream binding fields and cached-result references.
 *
 * The implementation converts @ref MnaDataKind and
 * @ref MnaPortDir through the canonical string helpers from
 * @ref mna_types.h so the on-disk vocabulary stays single-source.
 * Stream-related fields (@c stream_protocol, @c stream_endpoint,
 * @c stream_buffer_ms) and cache-related fields
 * (@c cached_result_path, @c cached_result_hash) are only written when
 * non-default so simple file-based projects stay compact, and any
 * unknown attribute is shovelled into @c extras to keep the round
 * trip lossless across mne_mna versions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_port.h"

#include "mna_codec.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaPort::to_json() const
{
    JsonObject json = extras;
    json["name"] = name;
    json["data_kind"] = data_kind_key(data_kind);
    json["direction"] = direction == MnaPortDir::Output ? "output" : "input";

    if (!source_node_id.empty())
        json["source_node"] = source_node_id;
    if (!source_port_name.empty())
        json["source_port"] = source_port_name;

    if (!stream_protocol.empty()) {
        json["stream_protocol"] = stream_protocol;
        json["stream_endpoint"] = stream_endpoint;
        json["stream_buffer_ms"] = stream_buffer_ms;
    }

    if (!cached_result_path.empty())
        json["cached_result"] = cached_result_path;
    if (!cached_result_hash.empty())
        json["cached_result_hash"] = cached_result_hash;

    return json;
}

//=============================================================================================================

MnaPort MnaPort::from_json(const JsonObject& json)
{
    MnaPort port;
    port.name = json["name"].to_std_string();
    port.data_kind = data_kind_from_string(json["data_kind"].to_string());
    port.direction = json["direction"].to_string() == "output" ? MnaPortDir::Output : MnaPortDir::Input;
    port.source_node_id = json["source_node"].to_std_string();
    port.source_port_name = json["source_port"].to_std_string();
    port.stream_protocol = json["stream_protocol"].to_std_string();
    port.stream_endpoint = json["stream_endpoint"].to_std_string();
    port.stream_buffer_ms = json["stream_buffer_ms"].to_int(0);
    port.cached_result_path = json["cached_result"].to_std_string();
    port.cached_result_hash = json["cached_result_hash"].to_std_string();
    port.extras = json.without({"name", "data_kind", "direction", "source_node", "source_port", "stream_protocol",
                                "stream_endpoint", "stream_buffer_ms", "cached_result", "cached_result_hash"});
    return port;
}

//=============================================================================================================

CborMap MnaPort::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaPort MnaPort::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
