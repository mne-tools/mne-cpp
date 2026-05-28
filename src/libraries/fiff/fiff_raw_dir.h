//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2012-2026 MNE-CPP Authors
 *
 * @file     fiff_raw_dir.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Christof Pieloth <pieloth@labp.htwk-leipzig.de>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     September 2012
 * @brief    One entry of the per-buffer raw-data directory: kind, first sample, number of samples and on-disk position.
 *
 * For every @c FIFF_DATA_BUFFER tag in a continuous recording
 * @ref FiffStream builds one @ref FiffRawDir entry: the kind of buffer
 * (int16 packed, float32, ...), the first sample index of the buffer in
 * the recording's sample timeline, the number of samples it contains, and
 * the on-disk byte position of the underlying tag header. Together those
 * entries form the sample → file-position index that
 * @c FiffRawData::read_segment uses for random-access decoding.
 */

#ifndef FIFF_RAW_DIR_H
#define FIFF_RAW_DIR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"
#include "fiff_types.h"
#include "fiff_dir_entry.h"

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
 * @brief Per-buffer raw-data directory entry: data kind, first sample, sample count, on-disk tag position.
 *
 * One entry per @c FIFF_DATA_BUFFER tag. The (@c first, @c nsamp) fields
 * let @c FiffRawData::read_segment binary-search the directory for the
 * buffers that cover a requested sample window and stream them in
 * without rescanning the file.
 */
class FIFFSHARED_EXPORT FiffRawDir {

public:
    using SPtr = QSharedPointer<FiffRawDir>;            /**< Shared pointer type for FiffRawDir. */
    using ConstSPtr = QSharedPointer<const FiffRawDir>; /**< Const shared pointer type for FiffRawDir. */
    using UPtr = std::unique_ptr<FiffRawDir>;             /**< Unique pointer type for FiffRawDir. */
    using ConstUPtr = std::unique_ptr<const FiffRawDir>;  /**< Const unique pointer type for FiffRawDir. */

    //=========================================================================================================
    /**
     * Default constructor
     */
    FiffRawDir();

    //=========================================================================================================
    /**
     * Copy constructor.
     *
     * @param[in] p_FiffRawDir   Raw directory entry which should be copied.
     */
    FiffRawDir(const FiffRawDir &p_FiffRawDir);

    //=========================================================================================================
    /**
     * Destroys the raw Directory entry.
     */
    ~FiffRawDir();

public:
    FiffDirEntry::SPtr  ent;    /**< Directory entry description. */
    fiff_int_t          first;  /**< first sample. */
    fiff_int_t          last;   /**< last sample. */
    fiff_int_t          nsamp;  /**< Number of samples. */
};
} // NAMESPACE

#endif // FIFF_RAW_DIR_H
