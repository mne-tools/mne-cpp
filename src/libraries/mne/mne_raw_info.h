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
 * @brief    MNERawInfo class declaration.
 *
 */

#ifndef MNERAWINFO_H
#define MNERAWINFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

#include <fiff/fiff_dir_node.h>
#include <fiff/fiff_stream.h>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

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
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB { class FiffCoordTrans; }

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * @brief Information about raw data in a FIFF file.
 */
class MNESHARED_EXPORT MNERawInfo
{
public:
    typedef QSharedPointer<MNERawInfo> SPtr;              /**< Shared pointer type for MNERawInfo. */
    typedef QSharedPointer<const MNERawInfo> ConstSPtr;   /**< Const shared pointer type for MNERawInfo. */

    //=========================================================================================================
    /**
     * @brief Constructs a default MNERawInfo.
     */
    MNERawInfo();

    //=========================================================================================================
    /**
     * @brief Destroys the MNERawInfo and releases owned resources.
     */
    ~MNERawInfo();

    /**
     * Finds the nearest FIFFB_MEAS parent node in the FIFF directory tree.
     *
     * Walks up the tree from the given node by following parent pointers until a
     * node of type FIFFB_MEAS is found.
     *
     * @param[in] node   Starting node from which to search upward.
     *
     * @return Shared pointer to the enclosing FIFFB_MEAS node, or an empty
     *         shared pointer if no such node is found.
     */
    static FIFFLIB::FiffDirNode::SPtr find_meas (const FIFFLIB::FiffDirNode::SPtr& node);

    /**
     * Finds the FIFFB_MEAS_INFO child node within the nearest FIFFB_MEAS block.
     *
     * First locates the enclosing FIFFB_MEAS node by walking up the tree from
     * the given node, then searches its immediate children for a node of type
     * FIFFB_MEAS_INFO.
     *
     * @param[in] node   Starting node from which to search.
     *
     * @return Shared pointer to the FIFFB_MEAS_INFO node, or an empty shared
     *         pointer if not found.
     */
    static FIFFLIB::FiffDirNode::SPtr find_meas_info (const FIFFLIB::FiffDirNode::SPtr& node);

    /**
     * Finds the raw data block in the FIFF directory tree.
     *
     * Searches the tree under the given node for a FIFFB_RAW_DATA block. If none
     * is found, falls back to searching for a FIFFB_CONTINUOUS_DATA block.
     *
     * @param[in] node   Root node of the subtree to search.
     *
     * @return Shared pointer to the raw data directory node, or an empty shared
     *         pointer if neither raw nor continuous data blocks are found.
     */
    static FIFFLIB::FiffDirNode::SPtr find_raw (const FIFFLIB::FiffDirNode::SPtr& node);

    /**
     * Finds the MaxShield raw data block in the FIFF directory tree.
     *
     * Searches the tree under the given node for a FIFFB_SMSH_RAW_DATA block,
     * which contains SSS/MaxShield processed raw data.
     *
     * @param[in] node   Root node of the subtree to search.
     *
     * @return Shared pointer to the MaxShield raw data directory node, or an
     *         empty shared pointer if no such block is found.
     */
    static FIFFLIB::FiffDirNode::SPtr find_maxshield (const FIFFLIB::FiffDirNode::SPtr& node);

    /**
     * Reads measurement information from the nearest FIFFB_MEAS_INFO parent of a node.
     *
     * Extracts channel information, sampling frequency, filter parameters,
     * coordinate transformation (device-to-head), and measurement ID from the
     * FIFF file. Lowpass and highpass default to sfreq/2 and 0 respectively if
     * not explicitly present in the file.
     *
     * @param[in]  stream      Open FIFF stream to read tags from.
     * @param[in]  node        Starting directory node (typically the raw data node).
     * @param[out] id          Measurement ID (managed via unique_ptr). Reset if absent.
     * @param[out] nchan       Number of channels.
     * @param[out] sfreq       Sampling frequency in Hz.
     * @param[out] highpass    Highpass filter cutoff frequency in Hz.
     * @param[out] lowpass     Lowpass filter cutoff frequency in Hz.
     * @param[out] chp         List of channel information structures, one per channel.
     * @param[out] trans       Device-to-head coordinate transformation.
     * @param[out] start_time  Measurement start time (NULL if absent).
     *
     * @return 0 on success, -1 on failure.
     */
    static int get_meas_info (FIFFLIB::FiffStream::SPtr& stream,
                              FIFFLIB::FiffDirNode::SPtr& node,
                              std::unique_ptr<FIFFLIB::FiffId>& id,
                              int *nchan,
                              float *sfreq,
                              float *highpass,
                              float *lowpass,
                              QList<FIFFLIB::FiffChInfo>& chp,
                              FIFFLIB::FiffCoordTrans& trans,
                              FIFFLIB::FiffTime* *start_time);

    /**
     * Loads raw data information from a FIFF file.
     *
     * Opens the specified FIFF file, locates the raw data block (or MaxShield
     * data block if allowed and no standard raw data is found), reads the
     * measurement information, determines the buffer size from the first data
     * buffer tag, and assembles a complete MNERawInfo structure.
     *
     * @param[in]  name              Path to the FIFF file to load.
     * @param[in]  allow_maxshield   If non-zero, fall back to MaxShield (SSS) data
     *                               when no standard raw data block is found.
     * @param[out] infop             Pointer to receive the newly allocated MNERawInfo
     *                               structure. Only valid on success.
     *
     * @return FIFF_OK on success, FIFF_FAIL on failure.
     */
    static int load(const QString& name, int allow_maxshield, std::unique_ptr<MNERawInfo>& infop);

public:
    QString             filename;      /**< The name of the file this comes from. */
    std::unique_ptr<FIFFLIB::FiffId> id;  /**< Measurement id from the file. */
    int                 nchan;          /**< Number of channels. */
    QList<FIFFLIB::FiffChInfo> chInfo;         /**< Channel info data . */
    int                 coord_frame;    /**< Which coordinate frame are the
                                         * positions defined in?
                                         */
    std::unique_ptr<FIFFLIB::FiffCoordTrans> trans;  /**< Coordinate transformation FIFF_COORD_HEAD <-> FIFF_COORD_DEVICE. */
    float         sfreq;          /**< Sampling frequency. */
    float         lowpass;        /**< Lowpass filter setting. */
    float         highpass;       /**< Highpass filter setting. */
    FIFFLIB::FiffTime      start_time;    /**< Acquisition start time (from meas date or block id). */
    int         buf_size;       /**< Buffer size in samples. */
    int         maxshield_data; /**< Are these unprocessed MaxShield data. */
    QList<FIFFLIB::FiffDirEntry::SPtr>  rawDir; /**< Directory of raw data tags (FIFF_DATA_BUFFER, FIFF_DATA_SKIP, etc.). */
    int           ndir;       /**< Number of tags in the above directory. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNERAWINFO_H
