//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mna_io.h
 * @since April 2026
 * @brief Reader and writer for MNA project containers — @c .mna (JSON, human-editable) and @c .mnx (CBOR, compact binary with @c "MNX1" magic).
 *
 * @ref MnaIO is the single dispatch point between an on-disk
 * project file and an in-memory @ref MnaProject. The public
 * @ref MnaIO::read and @ref MnaIO::write helpers pick the right
 * codec by extension so callers never have to branch: @c .mna
 * routes to a UTF-8 JSON serialiser intended for diff-friendly
 * version control and hand editing, while @c .mnx routes to a CBOR
 * stream prefixed with the four-byte @c "MNX1" magic for fast
 * loading of large projects with thousands of file references.
 *
 * Both codecs are lossless and symmetric: a JSON project written,
 * read back, re-serialised as CBOR and read again must yield the
 * same @ref MnaProject, including unknown keys preserved via
 * @c extras on every nested struct. That guarantee is what lets
 * collaborators on different MNALIB versions exchange projects
 * without silently dropping fields.
 */

#ifndef MNA_IO_H
#define MNA_IO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mna_global.h"
#include "mna_project.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNALIB
//=============================================================================================================

namespace MNALIB{

//=============================================================================================================
/**
 * Read/write MNA project files (.mna = JSON, .mnx = CBOR with "MNX1" magic header).
 */
class MNASHARED_EXPORT MnaIO
{
public:
    //=========================================================================================================
    /**
     * Read an MNA project from file. Dispatches by extension.
     *
     * @param[in] path   Path to the .mna or .mnx file.
     *
     * @return The deserialized MnaProject.
     */
    static MnaProject read(const QString& path);

    //=========================================================================================================
    /**
     * Write an MNA project to file. Dispatches by extension.
     *
     * @param[in] project   The project to write.
     * @param[in] path      Path to the .mna or .mnx file.
     *
     * @return True if successful.
     */
    static bool write(const MnaProject& project, const QString& path);

private:
    static MnaProject readJson(const QString& path);
    static bool       writeJson(const MnaProject& project, const QString& path);
    static MnaProject readCbor(const QString& path);
    static bool       writeCbor(const MnaProject& project, const QString& path);
};

} // namespace MNALIB

#endif // MNA_IO_H
