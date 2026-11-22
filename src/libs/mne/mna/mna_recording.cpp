//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_recording.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaRecording — ordered @ref MnaFileRef list plus opaque id and forward-compatible @c extras.
 *
 * The implementation defers to @ref MnaFileRef::to_json and the
 * matching @c from_cbor for each file entry, preserving order so
 * the first @c MnaFileRole::Raw element keeps its conventional
 * meaning as the canonical recording artefact. Unknown keys are
 * routed through @c extras on read/write so newer fields added by
 * tooling round-trip cleanly through this build.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_recording.h"

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

/** @return The JSON form without the files, which differ between JSON and CBOR. */
JsonObject metadata(const MnaRecording& item)
{
    JsonObject json = item.extras;
    json["id"] = item.id;
    return json;
}

/** @return The recording described by @p json, without the files. */
MnaRecording from_metadata(const JsonObject& json)
{
    MnaRecording item;
    item.id = json["id"].to_std_string();
    item.extras = json.without({"id", "files"});
    return item;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaRecording::to_json() const
{
    JsonObject json = metadata(*this);
    json["files"] = detail::to_json_array(files);
    return json;
}

//=============================================================================================================

MnaRecording MnaRecording::from_json(const JsonObject& json)
{
    MnaRecording item = from_metadata(json);
    item.files = detail::from_json_array<MnaFileRef>(json["files"]);
    return item;
}

//=============================================================================================================

CborMap MnaRecording::to_cbor() const
{
    CborMap cbor = CborMap::from_json_object(metadata(*this));
    cbor.insert("files", detail::to_cbor_array(files));
    return cbor;
}

//=============================================================================================================

MnaRecording MnaRecording::from_cbor(const CborMap& cbor)
{
    MnaRecording item = from_metadata(cbor.to_json_object({"files"}));
    item.files = detail::from_cbor_array<MnaFileRef>(cbor["files"]);
    return item;
}
