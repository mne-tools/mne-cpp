//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_file_ref.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Reference to a file inside an MNA project — relative path, semantic role, SHA-256 hash, and optional embedded payload.
 *
 * @ref mne::MnaFileRef is the leaf node of an MNA project tree. Every raw
 * recording, forward/inverse operator, covariance matrix, BEM model
 * or annotation that belongs to a subject is recorded as one of
 * these structures, never as a bare path string, so the container
 * always knows what the file @em is (via @ref mne::MnaFileRole "MnaFileRole"), what
 * format it carries (FIFF, MGH, STC, …), how big it is, and whether
 * its bytes have changed since the project was last saved.
 *
 * The @c sha256 field is the contract that makes pipelines
 * reproducible: the graph executor refuses to consume an input
 * whose digest no longer matches and downstream cached results are
 * invalidated automatically. The @c embedded flag plus inline
 * @c data buffer let a project be self-contained when shipped to a
 * collaborator (think "single-file analysis bundle"), at the cost
 * of larger container size. @c extras preserves any keys produced
 * by newer tooling so old readers can still round-trip projects
 * without dropping unknown metadata.
 */

#ifndef MNA_FILE_REF_H
#define MNA_FILE_REF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_types.h"

#include <mne/base/cbor.h>
#include <mne/base/error.h>
#include <mne/base/json.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Reference to a file inside an MNA project, optionally with its content embedded.
 *
 * Embedded data is base64 text in .mna (JSON) files and a byte string in .mnx (CBOR) files.
 *
 * @snippet ex_mna/main.cpp mna_project_files
 */
struct MNASHARED_EXPORT MnaFileRef
{
    MnaFileRole role = MnaFileRole::Custom; /**< Role of the file. */
    std::string path;                       /**< Relative POSIX path. */
    std::string sha256;                     /**< SHA-256 hash of file contents. */
    std::string format;                     /**< File format: "fiff", "mgh", "stc", etc. */
    std::int64_t size_bytes = 0;            /**< File size in bytes. */
    bool embedded = false;                  /**< Whether data is embedded in the container. */
    std::vector<std::byte> data;            /**< Embedded file data (only when embedded == true). */
    JsonObject extras;                      /**< Unknown keys preserved for lossless round-trip. */

    /** @return The reference as a JSON object, embedded data as base64. */
    JsonObject to_json() const;

    /** @param[in] json   A reference as written by to_json(). @return The reference. */
    static MnaFileRef from_json(const JsonObject& json);

    /** @return The reference as a CBOR map, embedded data as a byte string. */
    CborMap to_cbor() const;

    /** @param[in] cbor   A reference as written by to_cbor(). @return The reference. */
    static MnaFileRef from_cbor(const CborMap& cbor);

    //=========================================================================================================
    /**
     * Embeds the content of a file: sets data, size_bytes and embedded; path is left to the caller.
     *
     * @param[in] file   The file to read.
     *
     * @return Nothing, or why the file cannot be read; the reference is unchanged then.
     */
    Expected<void> embed(const std::filesystem::path& file);

    //=========================================================================================================
    /**
     * Writes the embedded data to a file and turns this into a reference to it: clears data and embedded;
     * path is left to the caller.
     *
     * @param[in] file   The file to write.
     *
     * @return Nothing, or why the file cannot be written; the reference is unchanged then.
     */
    Expected<void> extract(const std::filesystem::path& file);
};

} // namespace mne

#endif // MNA_FILE_REF_H
