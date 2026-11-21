//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2017-2026 MNE-CPP Authors
 *
 * @file     mne_raw_info.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Juan GPC <jgarciaprieto@mgh.harvard.edu>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.0
 * @date     January 2017
 * @brief    Subset of FIFF measurement info needed by the legacy raw-data path.
 *
 * @ref mne::MNERawInfo captures the sampling frequency, channel list,
 * calibration vector and the @ref mne::MNERawBufDef table reconstructed from
 * a @c -raw.fif file - effectively the fields that the original MNE C
 * tools stored next to the file handle. Provides quick metadata access
 * without forcing callers to materialise a full @ref mne::FiffInfo.
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
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <string>
#include <memory>
#include <vector>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace mne
{
class FiffCoordTrans;
}

//=============================================================================================================
// DEFINE NAMESPACE mne
//=============================================================================================================

namespace mne
{

//=============================================================================================================
/**
 * @brief Information about raw data in a FIFF file.
 *
 * @snippet ex_mne_api/main.cpp mne_raw_data_usage
 */
class MNESHARED_EXPORT MNERawInfo
{
public:
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
    static std::shared_ptr<mne::FiffDirNode> find_meas(const std::shared_ptr<mne::FiffDirNode>& node);

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
    static std::shared_ptr<mne::FiffDirNode> find_meas_info(const std::shared_ptr<mne::FiffDirNode>& node);

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
    static std::shared_ptr<mne::FiffDirNode> find_raw(const std::shared_ptr<mne::FiffDirNode>& node);

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
    static std::shared_ptr<mne::FiffDirNode> find_maxshield(const std::shared_ptr<mne::FiffDirNode>& node);

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
    static int get_meas_info(mne::FiffStream& stream,
                             std::shared_ptr<mne::FiffDirNode>& node,
                             std::unique_ptr<mne::FiffId>& id,
                             int* nchan,
                             float* sfreq,
                             float* highpass,
                             float* lowpass,
                             std::vector<mne::FiffChInfo>& chp,
                             mne::FiffCoordTrans& trans,
                             mne::FiffTime& start_time);

    /**
     * @overload
     * Overload that does not return the measurement start time.
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
     *
     * @return 0 on success, -1 on failure.
     */
    static int get_meas_info(mne::FiffStream& stream,
                             std::shared_ptr<mne::FiffDirNode>& node,
                             std::unique_ptr<mne::FiffId>& id,
                             int* nchan,
                             float* sfreq,
                             float* highpass,
                             float* lowpass,
                             std::vector<mne::FiffChInfo>& chp,
                             mne::FiffCoordTrans& trans);

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
    static int load(const std::filesystem::path& name, int allow_maxshield, std::unique_ptr<MNERawInfo>& infop);

public:
    std::string filename;                       /**< The name of the file this comes from. */
    std::unique_ptr<mne::FiffId> id;            /**< Measurement id from the file. */
    int nchan;                                  /**< Number of channels. */
    std::vector<mne::FiffChInfo> chInfo;        /**< Channel info data . */
    int coord_frame;                            /**< Which coordinate frame are the
                                         * positions defined in?
                                         */
    std::unique_ptr<mne::FiffCoordTrans> trans; /**< Coordinate transformation FIFF_COORD_HEAD <-> FIFF_COORD_DEVICE. */
    float sfreq;                                /**< Sampling frequency. */
    float lowpass;                              /**< Lowpass filter setting. */
    float highpass;                             /**< Highpass filter setting. */
    mne::FiffTime start_time;                   /**< Acquisition start time (from meas date or block id). */
    int buf_size;                               /**< Buffer size in samples. */
    int maxshield_data;                         /**< Are these unprocessed MaxShield data. */
    std::vector<mne::FiffDirEntry> rawDir;      /**< Directory of raw data tags (FIFF_DATA_BUFFER, FIFF_DATA_SKIP, etc.). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE mne_mne

#endif // MNERAWINFO_H
