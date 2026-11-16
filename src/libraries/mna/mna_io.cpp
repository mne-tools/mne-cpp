//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_io.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of the @ref MnaIO codecs — UTF-8 JSON for @c .mna files and CBOR (prefixed with the @c "MNX1" four-byte magic) for @c .mnx files.
 *
 * The @ref read / @ref write entry points dispatch on the file
 * extension and delegate to the four private helpers. The CBOR
 * writer emits the @c MNX1 magic up-front so the reader can
 * fast-fail on a mismatched binary before attempting to decode
 * the payload; the JSON writer pretty-prints with indentation so
 * @c .mna files remain diff- and human-friendly under version
 * control. All four helpers share a strict error-handling policy:
 * file or parse errors surface as empty projects with a logged
 * warning so callers cannot silently confuse load failures
 * with empty input.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_io.h"

#include <base/byte_stream.h>
#include <base/cbor.h>
#include <base/json.h>
#include <base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;

//=============================================================================================================
// DEFINE STATICS AND LOCAL METHODS
//=============================================================================================================

namespace
{

constexpr std::array kMagic{std::byte{'M'}, std::byte{'N'}, std::byte{'X'}, std::byte{'1'}};

/** @return The major number of a version like "1.0". */
int majorVersion(std::string_view version)
{
    return to_int(version.substr(0, version.find('.'))).value_or(0);
}

/** @return @p project, or an error if its schema version has another major number than this build. */
Expected<MnaProject> checkVersion(MnaProject project, const std::filesystem::path& path)
{
    if (majorVersion(project.mna_version) != majorVersion(MnaProject::CURRENT_SCHEMA_VERSION)) {
        return fail(ErrorCode::Format, "{} has the incompatible MNA version '{}' (expected major {})", utf8_from_path(path), project.mna_version,
                    majorVersion(MnaProject::CURRENT_SCHEMA_VERSION));
    }
    return project;
}

} // namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Expected<MnaProject> MnaIO::read(const std::filesystem::path& path)
{
    const std::string extension = to_lower(utf8_from_path(path.extension()));
    if (extension == ".mna") {
        const Expected<std::string> text = read_text_file(path);
        if (!text) {
            return std::unexpected(text.error());
        }
        JsonParseError error;
        const JsonDocument doc = JsonDocument::from_json(*text, &error);
        if (error.error != JsonParseError::NoError) {
            return fail(ErrorCode::Format, "{} is not valid JSON: {} at byte {}", utf8_from_path(path), error.error_string(), error.offset);
        }
        return checkVersion(MnaProject::from_json(doc.object()), path);
    }
    if (extension == ".mnx") {
        const Expected<std::vector<std::byte>> bytes = read_file_bytes(path);
        if (!bytes) {
            return std::unexpected(bytes.error());
        }
        const std::span<const std::byte> data(*bytes);
        if (data.size() < kMagic.size() || !std::ranges::equal(data.first(kMagic.size()), kMagic)) {
            return fail(ErrorCode::Format, "{} does not start with the MNX1 magic bytes", utf8_from_path(path));
        }
        CborParseError error;
        const CborValue root = CborValue::from_cbor(data.subspan(kMagic.size()), &error);
        if (error.error != CborParseError::NoError || !root.is_map()) {
            return fail(ErrorCode::Format, "{} does not hold a CBOR map: {}", utf8_from_path(path), error.error_string());
        }
        return checkVersion(MnaProject::from_cbor(root.to_map()), path);
    }
    return fail(ErrorCode::Format, "{} is neither a .mna nor a .mnx file", utf8_from_path(path));
}

//=============================================================================================================

Expected<void> MnaIO::write(const MnaProject& project, const std::filesystem::path& path)
{
    const std::string extension = to_lower(utf8_from_path(path.extension()));
    if (extension == ".mna") {
        return write_text_file(path, JsonDocument(project.to_json()).to_json(JsonDocument::Indented));
    }
    if (extension != ".mnx") {
        return fail(ErrorCode::Format, "{} is neither a .mna nor a .mnx file", utf8_from_path(path));
    }
    return write_file_bytes(path, to_mnx(project));
}

//=============================================================================================================

std::vector<std::byte> MnaIO::to_mnx(const MnaProject& project)
{
    std::vector<std::byte> bytes(kMagic.begin(), kMagic.end());
    const CborByteArray cbor = CborValue(project.to_cbor()).to_cbor();
    bytes.insert(bytes.end(), cbor.begin(), cbor.end());
    return bytes;
}
