//=============================================================================================================
/**
 * @file     mne_raw_info.h
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
 * @brief    MneRawInfo class declaration.
 *
 */

#ifndef MNERAWINFO_H
#define MNERAWINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../mne_global.h"
#include <fiff/c/fiff_coord_trans_old.h>

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

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
 * Implements the MNE Raw Information (Replaces mneRawInfoRec, *mneRawInfo; struct of MNE-C mne_types.h).
 *
 * @brief Information about raw data in fiff file
 */
class MNESHARED_EXPORT MneRawInfo
{
public:
    typedef QSharedPointer<MneRawInfo> SPtr;              /**< Shared pointer type for MneRawInfo. */
    typedef QSharedPointer<const MneRawInfo> ConstSPtr;   /**< Const shared pointer type for MneRawInfo. */

    //=========================================================================================================
    /**
     * Constructs the MNE Raw Info
     * Refactored:  (.c)
     */
    MneRawInfo();

    //=========================================================================================================
    /**
     * Destroys the MNE Raw Info
     * Refactored: mne_free_raw_info (mne_raw_routines.c)
     */
    ~MneRawInfo();

    //============================= read_ch_info.c =============================

    static FIFFLIB::FiffDirNode::SPtr find_meas (const FIFFLIB::FiffDirNode::SPtr& node);

    static FIFFLIB::FiffDirNode::SPtr find_meas_info (const FIFFLIB::FiffDirNode::SPtr& node);

    static FIFFLIB::FiffDirNode::SPtr find_raw (const FIFFLIB::FiffDirNode::SPtr& node);

    static FIFFLIB::FiffDirNode::SPtr find_maxshield (const FIFFLIB::FiffDirNode::SPtr& node);

    static int get_meas_info (//fiffFile file,	 /* The file we are reading */
                              FIFFLIB::FiffStream::SPtr& stream,
                              FIFFLIB::FiffDirNode::SPtr& node,	 /* The directory node containing our data */
                              FIFFLIB::fiffId *id,	         /* The block id from the nearest FIFFB_MEAS
                                                                                  parent */
                              int *nchan,	         /* Number of channels */
                              float *sfreq,	         /* Sampling frequency */
                              float *highpass,       /* Highpass filter freq. */
                              float *lowpass,        /* Lowpass filter setting */
                              QList<FIFFLIB::FiffChInfo>& chp,	 /* Channel descriptions */
                              FIFFLIB::FiffCoordTransOld* *trans, /* Coordinate transformation
                                                                                  (head <-> device) */
                              FIFFLIB::fiffTime *start_time);

    static int mne_load_raw_info(const QString& name,int allow_maxshield,MneRawInfo* *infop);

public:
    QString             filename;      /**< The name of the file this comes from. */
    FIFFLIB::fiffId     id;             /**< Measurement id from the file. */
    int                 nchan;          /**< Number of channels. */
    QList<FIFFLIB::FiffChInfo> chInfo;         /**< Channel info data . */
    int                 coord_frame;    /**< Which coordinate frame are the
                                         * positions defined in?
                                         */
    FIFFLIB::FiffCoordTransOld* trans; /**< This is the coordinate transformation
                                             * FIFF_COORD_HEAD <--> FIFF_COORD_DEVICE
                                             */
    float         sfreq;          /**< Sampling frequency. */
    float         lowpass;        /**< Lowpass filter setting. */
    float         highpass;       /**< Highpass filter setting. */
    FIFFLIB::fiffTimeRec   start_time;    /**< Starting time of the acquisition
                                             * taken from the meas date
                                             * or the meas block id
                                             * whence it may be inaccurate. */
    int         buf_size;       /**< Buffer size in samples. */
    int         maxshield_data; /**< Are these unprocessed MaxShield data. */
    QList<FIFFLIB::FiffDirEntry::SPtr>  rawDir; /**< Directory of raw data tags
                                                     * These may be of type
                                                     *       FIFF_DATA_BUFFER
                                                     *       FIFF_DATA_SKIP
                                                     *       FIFF_DATA_SKIP_SAMP
                                                     *       FIFF_NOP
                                                     */
    int           ndir;       /**< Number of tags in the above directory. */

//// ### OLD STRUCT ###
//typedef struct {        /**< Information about raw data in fiff file. */
//    char                *filename;      /**< The name of the file this comes from. */
//    FIFFLIB::fiffId     id;             /**< Measurement id from the file. */
//    int                 nchan;          /**< Number of channels. */
//    FIFFLIB::fiffChInfo chInfo;         /**< Channel info data . */
//    int                 coord_frame;    /**< Which coordinate frame are the
//                                         * positions defined in?
//                                         */
//    MNELIB::FiffCoordTransOld* trans; /**< This is the coordinate transformation
//                                             * FIFF_COORD_HEAD <--> FIFF_COORD_DEVICE
//                                             */
//    float         sfreq;          /**< Sampling frequency. */
//    float         lowpass;        /**< Lowpass filter setting. */
//    float         highpass;       /**< Highpass filter setting. */
//    FIFFLIB::fiffTimeRec   start_time;    /**< Starting time of the acquisition
//                                             * taken from the meas date
//                                             * or the meas block id
//                                             * whence it may be inaccurate. */
//    int         buf_size;       /**< Buffer size in samples. */
//    int         maxshield_data; /**< Are these unprocessed MaxShield data. */
//    QList<FIFFLIB::FiffDirEntry::SPtr>  rawDir; /**< Directory of raw data tags
//                                                     * These may be of type
//                                                     *       FIFF_DATA_BUFFER
//                                                     *       FIFF_DATA_SKIP
//                                                     *       FIFF_DATA_SKIP_SAMP
//                                                     *       FIFF_NOP
//                                                     */
//    int           ndir;       /**< Number of tags in the above directory. */
//} mneRawInfoRec, *mneRawInfo;
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNERAWINFO_H
