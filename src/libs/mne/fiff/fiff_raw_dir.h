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
 * @ref mne::FiffStream builds one @ref mne::FiffRawDir entry: the kind of buffer
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
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Per-buffer raw-data directory entry: data kind, first sample, sample count, on-disk tag position.
 *
 * One entry per @c FIFF_DATA_BUFFER tag. The (@c first, @c nsamp) fields
 * let @c FiffRawData::read_segment binary-search the directory for the
 * buffers that cover a requested sample window and stream them in
 * without rescanning the file.
 *
 * @snippet ex_fiff_structure/main.cpp fiff_raw_dir_usage
 */
struct FIFFSHARED_EXPORT FiffRawDir
{
    FiffDirEntry ent;      /**< The data buffer; the default entry (kind -1) for a skip. */
    fiff_int_t first = -1; /**< First sample. */
    fiff_int_t last = -1;  /**< Last sample. */
    fiff_int_t nsamp = -1; /**< Number of samples. */

    /** @return Whether both describe the same samples of the same buffer. */
    bool operator==(const FiffRawDir&) const = default;
};
} // NAMESPACE

#endif // FIFF_RAW_DIR_H
