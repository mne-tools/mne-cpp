//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_brain_vision_reader.h
 * @since March 2026
 * @brief @ref BIDSLIB::AbstractFormatReader implementation for the BrainVision ``.vhdr / .vmrk / .eeg`` triplet.
 *
 * BrainVision is the de-facto wire format for sEEG / ECoG / scalp-EEG
 * recordings shared as BIDS datasets: an ASCII @c .vhdr describing
 * sampling rate, channel labels, calibration factors and the binary
 * layout; a parallel ASCII @c .vmrk carrying stimulus / response /
 * segment markers; and a flat @c .eeg payload holding the samples as
 * little-endian @c INT_16, @c INT_32 or @c IEEE_FLOAT_32 values, laid
 * out either @c MULTIPLEXED (channels interleaved at every time point)
 * or @c VECTORIZED (one contiguous channel after another).
 *
 * @ref BIDSLIB::BrainVisionReader parses the @c .vhdr / @c .vmrk once
 * on @ref BIDSLIB::BrainVisionReader::open, then services arbitrary
 * segment reads by seeking into the still-open @c .eeg file and
 * applying the per-channel @c resolution / unit scaling so callers
 * always see physical-unit values. The auxiliary @ref
 * BIDSLIB::BrainVisionMarker and @ref BIDSLIB::BrainVisionChannelInfo
 * structs expose the parsed header data unchanged for callers that
 * want to convert markers into BIDS events or channels into
 * @c _channels.tsv rows.
 *
 * Format reference: BrainVision Core Data Format 1.0
 * (https://www.brainproducts.com/downloads/more-software-tools/#bvcdf).
 */

#ifndef BIDS_BRAIN_VISION_READER_H
#define BIDS_BRAIN_VISION_READER_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_abstract_format_reader.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDateTime>
#include <QVector>
#include <QFile>
#include <QMap>

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Marker entry from the .vmrk file.
 */
struct BIDSSHARED_EXPORT BrainVisionMarker
{
    QString type;           /**< Marker type (e.g., "New Segment", "Stimulus", "Response"). */
    QString description;    /**< Description (e.g., "S253", "R255"). */
    long    position{0};    /**< 0-based sample position. */
    long    duration{0};    /**< Duration in samples (0 = point marker). */
    int     channel{0};     /**< Channel number (0 = all channels). */
    QDateTime date;         /**< Optional date/time (for New Segment markers). */
};

//=============================================================================================================
/**
 * @brief Channel info from the .vhdr header.
 */
struct BIDSSHARED_EXPORT BrainVisionChannelInfo
{
    int     channelNumber{0};
    QString name;
    QString reference;
    float   resolution{1.0f};   /**< Scaling factor (calibration value). */
    QString unit;               /**< Physical unit string (e.g. "µV", "V"). */

    FIFFLIB::FiffChInfo toFiffChInfo() const;
};

//=============================================================================================================
/**
 * @brief Binary format enumeration for BrainVision data files.
 */
enum class BVBinaryFormat {
    INT_16,
    INT_32,
    IEEE_FLOAT_32
};

//=============================================================================================================
/**
 * @brief Data orientation enumeration.
 */
enum class BVOrientation {
    MULTIPLEXED,
    VECTORIZED
};

//=============================================================================================================
/**
 * @brief The BrainVisionReader reads BrainVision file triplets (.vhdr/.vmrk/.eeg) and exposes
 *        them through the AbstractFormatReader interface.
 *
 *        BrainVision is the most common format for iEEG (sEEG/ECoG) recordings in BIDS datasets.
 *        The format stores data as little-endian binary (INT_16, INT_32, or IEEE_FLOAT_32),
 *        either MULTIPLEXED (channels interleaved per time point) or VECTORIZED (channels contiguous).
 */
class BIDSSHARED_EXPORT BrainVisionReader : public AbstractFormatReader
{
public:
    //=========================================================================================================
    /**
     * @brief Default constructor.
     */
    BrainVisionReader();

    ~BrainVisionReader() override;

    // AbstractFormatReader interface
    bool open(const QString& sFilePath) override;
    FIFFLIB::FiffInfo getInfo() const override;
    Eigen::MatrixXf readRawSegment(int iStartSampleIdx, int iEndSampleIdx) const override;
    long getSampleCount() const override;
    float getFrequency() const override;
    int getChannelCount() const override;
    FIFFLIB::FiffRawData toFiffRawData() const override;
    QString formatName() const override;
    bool supportsExtension(const QString& sExtension) const override;

    //=========================================================================================================
    /**
     * @brief Return all parsed markers from the .vmrk file.
     */
    QVector<BrainVisionMarker> getMarkers() const;

    //=========================================================================================================
    /**
     * @brief Return all channel infos.
     */
    QVector<BrainVisionChannelInfo> getChannelInfos() const;

    // Unit to scaling factor (relative to V)
    static float unitScale(const QString& sUnit);

private:
    bool parseHeader(const QString& sVhdrPath);
    bool parseMarkers(const QString& sVmrkPath);
    void computeSampleCount();

    QString m_sVhdrPath;
    QString m_sDataPath;
    QString m_sMarkerPath;

    // Header fields
    float           m_fSFreq{0.0f};
    int             m_iNumChannels{0};
    BVBinaryFormat  m_binaryFormat{BVBinaryFormat::INT_16};
    BVOrientation   m_orientation{BVOrientation::MULTIPLEXED};
    long            m_lSampleCount{0};

    QVector<BrainVisionChannelInfo> m_vChannels;
    QVector<BrainVisionMarker> m_vMarkers;

    mutable QFile m_dataFile;
    bool m_bIsOpen{false};
};

} // namespace BIDSLIB

#endif // BIDS_BRAIN_VISION_READER_H
