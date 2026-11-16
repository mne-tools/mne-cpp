//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_node.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaNode covering ports, attributes, IPC/Script configuration, verification, provenance and execution metadata.
 *
 * The implementation serialises every field declared in
 * @ref mna_node.h and routes nested structs through their own
 * codecs (@ref MnaPort, @ref MnaScript, @ref MnaVerification),
 * which keeps the per-node payload self-describing. The
 * @ref MnaNodeExecMode enum is encoded as a stable lowercase
 * string (@c batch / @c stream / @c ipc / @c script) so manual
 * edits to a @c .mna file remain readable. Conditional fields
 * (IPC parameters when @c exec_mode == @c Ipc, script body when
 * @c exec_mode == @c Script) are written only when meaningful so
 * the on-disk representation stays compact, while @c extras
 * preserves forward-compatible additions through the round-trip.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_node.h"

#include "mna_codec.h"

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

std::string_view execModeToString(MnaNodeExecMode mode)
{
    switch (mode) {
        case MnaNodeExecMode::Batch:
            return "batch";
        case MnaNodeExecMode::Stream:
            return "stream";
        case MnaNodeExecMode::Ipc:
            return "ipc";
        case MnaNodeExecMode::Script:
            return "script";
    }
    return "batch";
}

MnaNodeExecMode execModeFromString(std::string_view text)
{
    if (text == "stream")
        return MnaNodeExecMode::Stream;
    if (text == "ipc")
        return MnaNodeExecMode::Ipc;
    if (text == "script")
        return MnaNodeExecMode::Script;
    return MnaNodeExecMode::Batch;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaNode::to_json() const
{
    JsonObject json = extras;
    json["id"] = id;
    json["op_type"] = op_type;
    json["exec_mode"] = execModeToString(exec_mode);
    json["attributes"] = attributes;
    json["inputs"] = detail::to_json_array(inputs);
    json["outputs"] = detail::to_json_array(outputs);

    if (exec_mode == MnaNodeExecMode::Ipc) {
        json["ipc_command"] = ipc_command;
        json["ipc_args"] = JsonArray::from_string_list(ipc_args);
        json["ipc_work_dir"] = ipc_work_dir;
        json["ipc_transport"] = ipc_transport;
    }
    if (exec_mode == MnaNodeExecMode::Script) {
        json["script"] = script.to_json();
    }
    if (JsonObject verificationJson = verification.to_json(); !verificationJson.empty()) {
        json["verification"] = std::move(verificationJson);
    }
    if (!tool_version.empty())
        json["tool_version"] = tool_version;
    if (executed_at)
        json["executed_at"] = format_iso8601(*executed_at, false);
    json["dirty"] = dirty;
    return json;
}

//=============================================================================================================

MnaNode MnaNode::from_json(const JsonObject& json)
{
    MnaNode node;
    node.id = json["id"].to_std_string();
    node.op_type = json["op_type"].to_std_string();
    node.exec_mode = execModeFromString(json["exec_mode"].to_string());
    node.attributes = json["attributes"].to_object();
    node.inputs = detail::from_json_array<MnaPort>(json["inputs"]);
    node.outputs = detail::from_json_array<MnaPort>(json["outputs"]);
    node.ipc_command = json["ipc_command"].to_std_string();
    node.ipc_args = json["ipc_args"].to_array().to_string_list();
    node.ipc_work_dir = json["ipc_work_dir"].to_std_string();
    node.ipc_transport = json["ipc_transport"].to_std_string();
    if (json.contains("script")) {
        node.script = MnaScript::from_json(json["script"].to_object());
    }
    if (json.contains("verification")) {
        node.verification = MnaVerification::from_json(json["verification"].to_object());
    }
    node.tool_version = json["tool_version"].to_std_string();
    node.executed_at = parse_iso8601(json["executed_at"].to_string());
    node.dirty = json["dirty"].to_bool(true);
    node.extras = json.without({"id", "op_type", "exec_mode", "attributes", "inputs", "outputs", "ipc_command", "ipc_args",
                                "ipc_work_dir", "ipc_transport", "script", "verification", "tool_version", "executed_at",
                                "dirty"});
    return node;
}

//=============================================================================================================

CborMap MnaNode::to_cbor() const
{
    return detail::to_cbor_via_json(*this);
}

//=============================================================================================================

MnaNode MnaNode::from_cbor(const CborMap& cbor)
{
    return from_json(cbor.to_json_object());
}
