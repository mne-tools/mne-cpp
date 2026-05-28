//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_raw_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Subset of FIFF measurement info needed by the legacy raw-data path.
 *
 * @ref MNELIB::MNERawInfo captures the sampling frequency, channel list,
 * calibration vector and the @ref MNERawBufDef table reconstructed from
 * a @c -raw.fif file - effectively the fields that the original MNE C
 * tools stored next to the file handle. Provides quick metadata access
 * without forcing callers to materialise a full @ref FIFFLIB::FiffInfo.
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
     * @param[out] start_time  Measurement start time.  Populated from FIFF_MEAS_DATE
     *                           if present; otherwise from the measurement ID time;
     *                           otherwise zeroed.
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
                              FIFFLIB::FiffTime& start_time);

    /**
     * @overload
     * Overload that does not return the measurement start time.
     */
    static int get_meas_info (FIFFLIB::FiffStream::SPtr& stream,
                              FIFFLIB::FiffDirNode::SPtr& node,
                              std::unique_ptr<FIFFLIB::FiffId>& id,
                              int *nchan,
                              float *sfreq,
                              float *highpass,
                              float *lowpass,
                              QList<FIFFLIB::FiffChInfo>& chp,
                              FIFFLIB::FiffCoordTrans& trans);

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
