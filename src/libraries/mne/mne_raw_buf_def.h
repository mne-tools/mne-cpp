//=============================================================================================================
/**
 * @file     mne_raw_buf_def.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2017
 *
 * @section  LICENSE
 *
 * Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @brief    MNERawBufDef class declaration.
 *
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
