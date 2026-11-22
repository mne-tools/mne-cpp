//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_project.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Top-level MNA container binding subjects, processing pipeline, schema version and project metadata.
 *
 * @ref mne::MnaProject is the in-memory representation of one @c .mna
 * (JSON) or @c .mnx (CBOR) file and the entry point for every other
 * mne_mna type: it owns the @ref mne::MnaSubject list (subject → session
 * → recording → file) plus the @ref mne::MnaNode pipeline that describes
 * how derivatives are produced from raw inputs. Reading or writing
 * a project is a single @ref mne::MnaIO call away — @ref mne::MnaProject::read "read" and
 * @ref mne::MnaProject::write "write" are thin façades that dispatch on extension.
 *
 * The container is intentionally @em declarative: it records what
 * should be computed (operations, parameters, file references and
 * hashes) rather than embedding executable code, so collaborators
 * on different platforms can reproduce the analysis using their
 * own MNE-CPP build. @c mna_version / @ref mne::MnaProject::CURRENT_SCHEMA_VERSION "CURRENT_SCHEMA_VERSION"
 * are bumped on breaking schema changes, while @c extras preserves
 * forward-compatible additions so projects written by a newer
 * version round-trip losslessly through an older one.
 */

#ifndef MNA_PROJECT_H
#define MNA_PROJECT_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_node.h"
#include "mna_subject.h"

#include <mne/base/cbor.h>
#include <mne/base/error.h>
#include <mne/base/json.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Top-level MNA project container. Holds subjects, pipeline steps, and project metadata.
 *
 * @snippet ex_mna/main.cpp mna_project_save
 */
class MNASHARED_EXPORT MnaProject
{
public:
    /** Creates an empty project of the current schema version, created and modified now. */
    MnaProject();

    static constexpr std::string_view CURRENT_SCHEMA_VERSION = "1.0"; /**< Current MNA schema version. */

    std::string name;                 /**< Project name. */
    std::string description;          /**< Project description. */
    std::string mna_version;          /**< MNA schema version. */
    std::optional<UtcTime> created;   /**< Creation timestamp. */
    std::optional<UtcTime> modified;  /**< Last modification timestamp. */
    std::vector<MnaSubject> subjects; /**< Subjects in the project. */
    std::vector<MnaNode> pipeline;    /**< Processing pipeline nodes. */
    JsonObject extras;                /**< Unknown keys preserved for lossless round-trip. */

    /** @return The project as a JSON object, dates as ISO 8601 text. */
    JsonObject to_json() const;

    /** @param[in] json   A project as written by to_json(). @return The project; unknown keys go to extras. */
    static MnaProject from_json(const JsonObject& json);

    /** @return The project as a CBOR map, dates as tag 0 (ISO 8601 with milliseconds). */
    CborMap to_cbor() const;

    /** @param[in] cbor   A project as written by to_cbor(). @return The project; unknown keys go to extras. */
    static MnaProject from_cbor(const CborMap& cbor);

    /** @param[in] path   A .mna or .mnx file. @return The project, or why it cannot be read. Delegates to MnaIO. */
    static Expected<MnaProject> read(const std::filesystem::path& path);

    /** @param[in] path   A .mna or .mnx file. @return Nothing, or why it cannot be written. Delegates to MnaIO. */
    Expected<void> write(const std::filesystem::path& path) const;
};

} // namespace mne

#endif // MNA_PROJECT_H
