//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_channel.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Reader/writer for the BIDS ``_channels.tsv`` sidecar — one record per recorded channel.
 *
 * BIDS mandates that every electrophysiology recording (MEG, EEG, iEEG)
 * ship a sibling @c _channels.tsv with one row per channel describing
 * @c name, @c type and @c units (REQUIRED) plus optional sampling /
 * filter / status / description columns. @ref BIDSLIB::BidsChannel is the in-memory
 * row record and the static @ref BIDSLIB::BidsChannel::read_tsv "BidsChannel::read_tsv" /
 * @ref BIDSLIB::BidsChannel::write_tsv "BidsChannel::write_tsv" functions perform the TSV round-trip via
 * @ref BIDSLIB::BidsTsv. The BIDS channel-type vocabulary (@c MEGMAG, @c EEG,
 * @c ECOG, @c SEEG, @c DBS, @c TRIG, …) maps to FIFF channel kinds
 * through the tables in @ref bids_const.h, so a channel record read
 * from a BIDS dataset can be merged into a @c FIFFLIB::FiffInfo without
 * an additional translation layer.
 *
 * Spec: https://bids-specification.readthedocs.io/en/stable/modality-specific-files/electrophysiology.html
 */

#ifndef BIDS_CHANNEL_H
#define BIDS_CHANNEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <filesystem>
#include <span>
#include <string>
#include <vector>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Channel metadata record corresponding to one row in *_channels.tsv.
 *
 * @snippet ex_bids/main.cpp bids_sidecar_records
 */
struct BIDSSHARED_EXPORT BidsChannel
{
    std::string name;         /**< Channel name (REQUIRED). */
    std::string type;         /**< BIDS channel type: EEG, ECOG, SEEG, DBS, TRIG, etc. (REQUIRED). */
    std::string units;        /**< Unit of measurement: V, uV, mV (REQUIRED). */
    std::string samplingFreq; /**< Sampling frequency in Hz (OPTIONAL). */
    std::string lowCutoff;    /**< High-pass cutoff frequency in Hz (OPTIONAL). */
    std::string highCutoff;   /**< Low-pass cutoff frequency in Hz (OPTIONAL). */
    std::string notch;        /**< Notch filter frequency in Hz (OPTIONAL). */
    std::string status;       /**< "good" or "bad" (OPTIONAL). */
    std::string description;  /**< Free-text description (OPTIONAL). */

    /**
     * @brief Read a BIDS *_channels.tsv file.
     * @param[in] path       Path to the channels.tsv file.
     * @return List of channel records.
     */
    static std::vector<BidsChannel> read_tsv(const std::filesystem::path& path);

    /**
     * @brief Write a BIDS *_channels.tsv file.
     * @param[in] path       Output path.
     * @param[in] channels   List of channel records.
     * @return true on success.
     */
    static bool write_tsv(const std::filesystem::path& path,
                          std::span<const BidsChannel> channels);
};

} // namespace BIDSLIB

#endif // BIDS_CHANNEL_H
