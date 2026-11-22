//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_subject.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaSubject — subject id, FreeSurfer @c $SUBJECTS_DIR link, ordered session list and forward-compatible @c extras.
 *
 * The implementation walks the @ref MnaSession list with the
 * matching session codec and emits @c freesurfer_dir as a relative
 * POSIX path so projects remain portable between hosts. Any
 * subject-level fields not recognised by this build — demographics,
 * consent flags, clinical metadata — are routed through @c extras
 * to guarantee round-trip fidelity with newer mne_mna versions.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_subject.h"

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

/** @return The JSON form without the sessions, which differ between JSON and CBOR. */
JsonObject metadata(const MnaSubject& item)
{
    JsonObject json = item.extras;
    json["id"] = item.id;
    json["freesurfer_dir"] = item.freesurfer_dir;
    return json;
}

/** @return The subject described by @p json, without the sessions. */
MnaSubject from_metadata(const JsonObject& json)
{
    MnaSubject item;
    item.id = json["id"].to_std_string();
    item.freesurfer_dir = json["freesurfer_dir"].to_std_string();
    item.extras = json.without({"id", "freesurfer_dir", "sessions"});
    return item;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

JsonObject MnaSubject::to_json() const
{
    JsonObject json = metadata(*this);
    json["sessions"] = detail::to_json_array(sessions);
    return json;
}

//=============================================================================================================

MnaSubject MnaSubject::from_json(const JsonObject& json)
{
    MnaSubject item = from_metadata(json);
    item.sessions = detail::from_json_array<MnaSession>(json["sessions"]);
    return item;
}

//=============================================================================================================

CborMap MnaSubject::to_cbor() const
{
    CborMap cbor = CborMap::from_json_object(metadata(*this));
    cbor.insert("sessions", detail::to_cbor_array(sessions));
    return cbor;
}

//=============================================================================================================

MnaSubject MnaSubject::from_cbor(const CborMap& cbor)
{
    MnaSubject item = from_metadata(cbor.to_json_object({"sessions"}));
    item.sessions = detail::from_cbor_array<MnaSession>(cbor["sessions"]);
    return item;
}
