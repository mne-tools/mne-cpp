//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_raw_buf_def.h
 * @since March 2026
 * @brief Descriptor of a single @c FIFF_DATA_BUFFER block (file offset, sample range, type).
 *
 * @ref MNELIB::MNERawBufDef is the C++ port of @c mneRawBufDefRec. It
 * lets the raw reader perform random-access seek to any sample without
 * rescanning the file because each buffer's first sample, length and
 * FIFF datatype are precomputed once at open time.
 */

#ifndef MNERAWBUFDEF_H
#define MNERAWBUFDEF_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_dir_node.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
/**
 * @brief Definition of one raw data buffer within a FIFF file.
 *
 * Each MNERawBufDef describes a contiguous chunk of samples, either loaded
 * from the FIFF file or produced by the overlap-add filter pipeline.
 * Sample values are stored in a row-major matrix (nchan x ns) that is
 * managed by an external ring buffer; an empty (0x0) matrix indicates
 * that the data are not currently resident in memory.
 */
class MNESHARED_EXPORT MNERawBufDef
{
public:
    typedef QSharedPointer<MNERawBufDef> SPtr;              /**< Shared pointer type for MNERawBufDef. */
    typedef QSharedPointer<const MNERawBufDef> ConstSPtr;   /**< Const shared pointer type for MNERawBufDef. */

    //=========================================================================================================
    /**
     * @brief Default constructor.
     */
    MNERawBufDef();

    //=========================================================================================================
    /**
     * @brief Destructor.
     */
    ~MNERawBufDef();



public:
    FIFFLIB::FiffDirEntry::SPtr ent;    /**< Directory entry locating this buffer in the FIFF file (file buffers only). */
    int   firsts = 0;       /**< First sample index. */
    int   lasts = 0;        /**< Last sample index. */
    int   ntaper = 0;       /**< Taper length for filtered buffers. */
    int   ns = 0;           /**< Number of samples (lasts - firsts + 1). */
    int   nchan = 0;        /**< Number of channels. */
    bool  is_skip = false;     /**< True if this buffer represents a data skip. */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vals;  /**< Sample values matrix [nchan x ns], row-major (empty if not loaded). */
    bool  valid = false;        /**< True if the data in this buffer are meaningful. */
    Eigen::VectorXi ch_filtered; /**< Per-channel flag: has this channel been filtered already (filtered buffers only). */
    int   comp_status = 0;  /**< Compensation status for raw buffers. */
};

} // NAMESPACE MNELIB

#endif // MNERAWBUFDEF_H
