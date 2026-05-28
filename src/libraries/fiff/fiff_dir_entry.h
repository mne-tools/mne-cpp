//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file fiff_dir_entry.h
 * @since 2022
 * @date  March 2026
 * @brief Single entry of the FIFF tag directory: kind, type, byte size and absolute file position.
 *
 * After the last tag of a FIFF file the writer appends a @c FIFF_DIR tag
 * whose payload is an array of @c fiffDirEntryRec records, one per tag in
 * the stream, terminated by a sentinel with kind = -1. The directory
 * turns linear FIFF I/O into random-access: @ref FiffStream consults it to
 * locate any tag by kind without rescanning the file, and
 * @ref FiffDirNode uses it to materialize the block hierarchy
 * (@c FIFFB_BLOCK_START / @c FIFFB_BLOCK_END pairs) into a navigable tree.
 *
 * @ref FiffDirEntry is the in-memory mirror of that 16-byte record. It is
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
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE FIFFLIB
//=============================================================================================================

namespace FIFFLIB
{

//=============================================================================================================
/**
 * @brief Directory entry: tag kind + on-disk type + byte size + absolute file offset (16-byte record).
 *
 * Exact memory image of the legacy @c fiffDirEntryRec: kind (32-bit),
 * type (32-bit), size (32-bit), pos (32-bit). The arrays of these records
 * materialize the @c FIFF_DIR tag at the tail of every well-formed FIFF
 * file and drive random-access tag lookup in @ref FiffStream.
 */

class FIFFSHARED_EXPORT FiffDirEntry
{
public:
    using SPtr = QSharedPointer<FiffDirEntry>;            /**< Shared pointer type for FiffDirEntry. */
    using ConstSPtr = QSharedPointer<const FiffDirEntry>; /**< Const shared pointer type for FiffDirEntry. */
    using UPtr = std::unique_ptr<FiffDirEntry>;             /**< Unique pointer type for FiffDirEntry. */
    using ConstUPtr = std::unique_ptr<const FiffDirEntry>;  /**< Const unique pointer type for FiffDirEntry. */

    //=========================================================================================================
    /**
     * Constructs the dir entry.
     */
    FiffDirEntry();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffDirEntry   Dir Entry which should be copied.
     */
    FiffDirEntry(const FiffDirEntry& p_FiffDirEntry);

    //=========================================================================================================
    /**
     * Destroys the dir entry.
     */
    ~FiffDirEntry();

    //=========================================================================================================
    /**
     * Size of the old struct (fiffDirEntryRec) 4*int = 4*4 = 16
     *
     * @return the size of the old struct fiffDirEntryRec.
     */
    inline static qint32 storageSize();

public:
    fiff_int_t  kind;   /**< Tag number. */
    fiff_int_t  type;   /**< Data type. */
    fiff_int_t  size;   /**< How many bytes. */
    fiff_int_t  pos;    /**< Location in file; Note: the data is located at pos + FIFFC_DATA_OFFSET: 2GB restriction -> change this to fiff_long_t. */

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline qint32 FiffDirEntry::storageSize()
{
    return sizeof(FiffDirEntry::kind) + sizeof(FiffDirEntry::type)
         + sizeof(FiffDirEntry::size) + sizeof(FiffDirEntry::pos);
}
} // NAMESPACE

#endif // FIFF_DIR_ENTRY_H
