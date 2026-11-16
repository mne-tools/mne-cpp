//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mna_io.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Reader and writer for MNA project containers — @c .mna (JSON, human-editable) and @c .mnx (CBOR, compact binary with @c "MNX1" magic).
 *
 * @ref mne::MnaIO is the single dispatch point between an on-disk
 * project file and an in-memory @ref mne::MnaProject. The public
 * @ref mne::MnaIO::read "MnaIO::read" and @ref mne::MnaIO::write "MnaIO::write" helpers pick the right
 * codec by extension so callers never have to branch: @c .mna
 * routes to a UTF-8 JSON serialiser intended for diff-friendly
 * version control and hand editing, while @c .mnx routes to a CBOR
 * stream prefixed with the four-byte @c "MNX1" magic for fast
 * loading of large projects with thousands of file references.
 *
 * Both codecs are lossless and symmetric: a JSON project written,
 * read back, re-serialised as CBOR and read again must yield the
 * same @ref mne::MnaProject, including unknown keys preserved via
 * @c extras on every nested struct. That guarantee is what lets
 * collaborators on different mne_mna versions exchange projects
 * without silently dropping fields.
 */

#ifndef MNA_IO_H
#define MNA_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_project.h"

#include <base/error.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstddef>
#include <filesystem>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * Reads and writes MNA projects: .mna files hold indented JSON, .mnx files the magic bytes "MNX1" and CBOR.
 * Files of another major schema version are rejected.
 *
 * @brief Reader and writer for .mna (JSON) and .mnx (CBOR) project containers.
 *
 * @snippet ex_mna/main.cpp mna_project_save
 */
class MNASHARED_EXPORT MnaIO
{
public:
    /** @param[in] path   A .mna or .mnx file. @return The project, or why it cannot be read. */
    static Expected<MnaProject> read(const std::filesystem::path& path);

    /** @param[in] project   The project. @param[in] path   A .mna or .mnx file. @return Nothing, or why it cannot be written. */
    static Expected<void> write(const MnaProject& project, const std::filesystem::path& path);

    /** @param[in] project   The project. @return The bytes of a .mnx file, e.g. to offer as a download. */
    static std::vector<std::byte> to_mnx(const MnaProject& project);
};

} // namespace mne

#endif // MNA_IO_H
