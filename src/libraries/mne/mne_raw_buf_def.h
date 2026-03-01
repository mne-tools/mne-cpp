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
 * @brief    MneRawBufDef class declaration.
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
 * Implements the MNE Raw Information (Replaces *mneRawBufDef,mneRawBufDefRec; struct of MNE-C mne_types.h).
 *
 * @brief Information about raw data in fiff file
 */
class MNESHARED_EXPORT MneRawBufDef
{
public:
    typedef QSharedPointer<MneRawBufDef> SPtr;              /**< Shared pointer type for MneRawBufDef. */
    typedef QSharedPointer<const MneRawBufDef> ConstSPtr;   /**< Const shared pointer type for MneRawBufDef. */

    //=========================================================================================================
    /**
     * Constructs the MNE Raw Buffer Definition
     * Refactored:  (.c)
     */
    MneRawBufDef();

    //=========================================================================================================
    /**
     * Destroys the MNE Raw Buffer Definition
     * Refactored:  (.c)
     */
    ~MneRawBufDef();

    /**
     * Frees an array of raw data buffer definitions.
     *
     * Iterates through each buffer in the array, freeing the per-channel filter
     * flags (ch_filtered) and the value pointer array (vals). Note that only the
     * pointer array for vals is freed, not the underlying data which resides in
     * a shared ring buffer. Finally, the buffer array itself is freed.
     *
     * @param[in] bufs   Pointer to the array of MneRawBufDef structures to free.
     * @param[in] nbuf   Number of buffer definitions in the array.
     */
    static void free_bufs(MneRawBufDef* bufs, int nbuf);

public:
    FIFFLIB::FiffDirEntry::SPtr ent;    /**< Directory entry locating this buffer in the FIFF file (file buffers only). */
    int   firsts,lasts;     /**< First and last sample indices. */
    int   ntaper;           /**< Taper length for filtered buffers. */
    int   ns;               /**< Number of samples (lasts - firsts + 1). */
    int   nchan;            /**< Number of channels. */
    int   is_skip;          /**< Non-zero if this buffer represents a data skip. */
    float **vals;           /**< Sample values array [nchan x ns] (NULL if not loaded into memory). */
    int   valid;            /**< Non-zero if the data in this buffer are meaningful. */
    int   *ch_filtered;     /**< Per-channel flag: has this channel been filtered already (filtered buffers only). */
    int   comp_status;      /**< Compensation status for raw buffers. */

//// ### OLD STRUCT ###
//typedef struct {
//    FIFFLIB::FiffDirEntry::SPtr ent;    /* Where is this in the file (file bufs only, pointer to info) */
//    int   firsts,lasts;     /* First and last sample */
//    int   ntaper;           /* For filtered buffers: taper length */
//    int   ns;               /* Number of samples (last - first + 1) */
//    int   nchan;            /* Number of channels */
//    int   is_skip;          /* Is this a skip? */
//    float **vals;           /* Values (null if not in memory) */
//    int   valid;            /* Are the data meaningful? */
//    int   *ch_filtered;     /* For filtered buffers: has this channel filtered already */
//    int   comp_status;      /* For raw buffers: compensation status */
//} *mneRawBufDef,mneRawBufDefRec;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNERAWBUFDEF_H
