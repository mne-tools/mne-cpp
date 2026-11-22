//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_session.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaSession — ordered @ref MnaRecording list plus opaque id and forward-compatible @c extras.
 *
 * Implementation mirrors the rest of the project tree: each
 * recording is serialised through @ref MnaRecording::to_json /
 * @c to_cbor, ordering is preserved to keep run numbering stable,
 * and any unknown sidecar metadata supplied by newer tooling is
 * captured into @c extras on read and re-emitted on write so the
 * round-trip stays lossless across mne_mna versions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_session.h"

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

/** @return The JSON form without the recordings, which differ between JSON and CBOR. */
JsonObject metadata(const MnaSession& item)
{
    JsonObject json = item.extras;
    json["id"] = item.id;
    return json;
}

/** @return The session described by @p json, without the recordings. */
MnaSession from_metadata(const JsonObject& json)
{
    MnaSession item;
    item.id = json["id"].to_std_string();
    item.extras = json.without({"id", "recordings"});
    return item;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaSession::to_json() const
{
    JsonObject json = metadata(*this);
    json["recordings"] = detail::to_json_array(recordings);
    return json;
}

//=============================================================================================================

MnaSession MnaSession::from_json(const JsonObject& json)
{
    MnaSession item = from_metadata(json);
    item.recordings = detail::from_json_array<MnaRecording>(json["recordings"]);
    return item;
}

//=============================================================================================================

CborMap MnaSession::to_cbor() const
{
    CborMap cbor = CborMap::from_json_object(metadata(*this));
    cbor.insert("recordings", detail::to_cbor_array(recordings));
    return cbor;
}

//=============================================================================================================

MnaSession MnaSession::from_cbor(const CborMap& cbor)
{
    MnaSession item = from_metadata(cbor.to_json_object({"recordings"}));
    item.recordings = detail::from_cbor_array<MnaRecording>(cbor["recordings"]);
    return item;
}
