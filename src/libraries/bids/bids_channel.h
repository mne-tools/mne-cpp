//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_channel.h
 * @since 2026
 * @date  April 2026
 * @brief Reader/writer for the BIDS ``_channels.tsv`` sidecar — one record per recorded channel.
 *
 * BIDS mandates that every electrophysiology recording (MEG, EEG, iEEG)
 * ship a sibling @c _channels.tsv with one row per channel describing
 * @c name, @c type and @c units (REQUIRED) plus optional sampling /
 * filter / status / description columns. @ref BidsChannel is the in-memory
 * row record and the static @ref BidsChannel::readTsv /
 * @ref BidsChannel::writeTsv functions perform the TSV round-trip via
 * @ref BidsTsv. The BIDS channel-type vocabulary (@c MEGMAG, @c EEG,
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
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Channel metadata record corresponding to one row in *_channels.tsv.
 */
struct BIDSSHARED_EXPORT BidsChannel
{
    QString name;           /**< Channel name (REQUIRED). */
    QString type;           /**< BIDS channel type: EEG, ECOG, SEEG, DBS, TRIG, etc. (REQUIRED). */
    QString units;          /**< Unit of measurement: V, uV, mV (REQUIRED). */
    QString samplingFreq;   /**< Sampling frequency in Hz (OPTIONAL). */
    QString lowCutoff;      /**< High-pass cutoff frequency in Hz (OPTIONAL). */
    QString highCutoff;     /**< Low-pass cutoff frequency in Hz (OPTIONAL). */
    QString notch;          /**< Notch filter frequency in Hz (OPTIONAL). */
    QString status;         /**< "good" or "bad" (OPTIONAL). */
    QString description;    /**< Free-text description (OPTIONAL). */

    /**
     * @brief Read a BIDS *_channels.tsv file.
     * @param[in] sFilePath  Path to the channels.tsv file.
     * @return List of channel records.
     */
    static QList<BidsChannel> readTsv(const QString& sFilePath);

    /**
     * @brief Write a BIDS *_channels.tsv file.
     * @param[in] sFilePath  Output path.
     * @param[in] channels   List of channel records.
     * @return true on success.
     */
    static bool writeTsv(const QString& sFilePath,
                         const QList<BidsChannel>& channels);
};

} // namespace BIDSLIB

#endif // BIDS_CHANNEL_H
