//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_dir_entry.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    Single entry of the FIFF tag directory: kind, type, byte size and absolute file position.
 *
 * After the last tag of a FIFF file the writer appends a @c FIFF_DIR tag
 * whose payload is an array of @c fiffDirEntryRec records, one per tag in
 * the stream, terminated by a sentinel with kind = -1. The directory
 * turns linear FIFF I/O into random-access: @ref mne::FiffStream consults it to
 * locate any tag by kind without rescanning the file, and
 * @ref mne::FiffDirNode uses it to materialize the block hierarchy
 * (@c FIFFB_BLOCK_START / @c FIFFB_BLOCK_END pairs) into a navigable tree.
 *
 * @ref mne::FiffDirEntry is the in-memory mirror of that 16-byte record. It is
 * intentionally trivially copyable so the directory can be slurped in as
 * a single Eigen / Qt vector.
 */

#ifndef FIFF_DIR_ENTRY_H
#define FIFF_DIR_ENTRY_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cstdint>

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Directory entry: tag kind + on-disk type + byte size + absolute file offset (16-byte record).
 *
 * Exact memory image of the legacy @c fiffDirEntryRec record: kind (32-bit),
 * type (32-bit), size (32-bit), pos (32-bit). The arrays of these records
 * materialize the @c FIFF_DIR tag at the tail of every well-formed FIFF
 * file and drive random-access tag lookup in @ref FiffStream.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_dir_entry_usage
 *
 * @snippet ex_fiff_structure/main.cpp fiff_raw_dir_usage
 */

struct FiffDirEntry
{
    fiff_int_t kind = -1; /**< Tag number. */
    fiff_int_t type = -1; /**< Data type. */
    fiff_int_t size = -1; /**< How many bytes. */
    fiff_int_t pos = -1;  /**< Location in file; the data is located at pos + FIFFC_DATA_OFFSET. */

    /** @return The size of one entry in a FIFF directory tag. */
    static constexpr std::int32_t storage_size()
    {
        return 4 * sizeof(fiff_int_t);
    }

    friend bool operator==(const FiffDirEntry&, const FiffDirEntry&) = default;
};

} // NAMESPACE

#endif // FIFF_DIR_ENTRY_H
