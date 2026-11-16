//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_project.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    JSON and CBOR codecs for @ref MnaProject plus @ref read / @ref write thin façades that delegate to @ref MnaIO.
 *
 * The implementation serialises the project metadata
 * (name, description, @c mna_version, creation/modification
 * timestamps) followed by the @ref MnaSubject list and the
 * @ref MnaNode pipeline. The @c mna_version field is written from
 * @ref MnaProject::CURRENT_SCHEMA_VERSION so older readers can
 * refuse projects from a future schema, while @c extras carries
 * forward-compatible additions through the round-trip without
 * loss.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_project.h"

#include "mna_codec.h"
#include "mna_io.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <chrono>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

/** CBOR tag of a date and time as ISO 8601 text (RFC 8949 section 3.4.1). */
constexpr std::uint64_t kDateTimeTag = 0;

/** @return The JSON form without the dates, subjects and pipeline. */
JsonObject metadata(const MnaProject& project)
{
    JsonObject json = project.extras;
    json["mna_version"] = project.mna_version;
    json["name"] = project.name;
    json["description"] = project.description;
    return json;
}

/** @return The project described by @p json, without the dates, subjects and pipeline. */
MnaProject fromMetadata(const JsonObject& json)
{
    MnaProject project;
    project.mna_version = json["mna_version"].to_std_string();
    project.name = json["name"].to_std_string();
    project.description = json["description"].to_std_string();
    project.extras = json.without({"mna_version", "name", "description", "created", "modified", "subjects", "pipeline"});
    return project;
}

/** @return A tag 0 date, or text; nothing for anything else. */
std::optional<UtcTime> timeFromCbor(const CborValue& value)
{
    return parse_iso8601(value.is_tag() ? value.tagged_value().to_string() : value.to_string());
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MnaProject::MnaProject()
: mna_version(CURRENT_SCHEMA_VERSION)
, created(std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()))
, modified(created)
{
}

//=============================================================================================================

JsonObject MnaProject::to_json() const
{
    JsonObject json = metadata(*this);
    json["created"] = created ? format_iso8601(*created) : std::string();
    json["modified"] = modified ? format_iso8601(*modified) : std::string();
    json["subjects"] = detail::to_json_array(subjects);
    json["pipeline"] = detail::to_json_array(pipeline);
    return json;
}

//=============================================================================================================

MnaProject MnaProject::from_json(const JsonObject& json)
{
    MnaProject project = fromMetadata(json);
    project.created = parse_iso8601(json["created"].to_string());
    project.modified = parse_iso8601(json["modified"].to_string());
    project.subjects = detail::from_json_array<MnaSubject>(json["subjects"]);
    project.pipeline = detail::from_json_array<MnaNode>(json["pipeline"]);
    return project;
}

//=============================================================================================================

CborMap MnaProject::to_cbor() const
{
    CborMap cbor = CborMap::from_json_object(metadata(*this));
    const auto date = [](const std::optional<UtcTime>& time) {
        return time ? CborValue::tagged(kDateTimeTag, format_iso8601(*time, true)) : CborValue(nullptr);
    };
    cbor.insert("created", date(created));
    cbor.insert("modified", date(modified));
    cbor.insert("subjects", detail::to_cbor_array(subjects));
    cbor.insert("pipeline", detail::to_cbor_array(pipeline));
    return cbor;
}

//=============================================================================================================

MnaProject MnaProject::from_cbor(const CborMap& cbor)
{
    MnaProject project = fromMetadata(cbor.to_json_object({"subjects", "pipeline"}));
    project.created = timeFromCbor(cbor["created"]);
    project.modified = timeFromCbor(cbor["modified"]);
    project.subjects = detail::from_cbor_array<MnaSubject>(cbor["subjects"]);
    project.pipeline = detail::from_cbor_array<MnaNode>(cbor["pipeline"]);
    return project;
}

//=============================================================================================================

Expected<MnaProject> MnaProject::read(const std::filesystem::path& path)
{
    return MnaIO::read(path);
}

//=============================================================================================================

Expected<void> MnaProject::write(const std::filesystem::path& path) const
{
    return MnaIO::write(*this, path);
}
