//=============================================================================================================
/**
 * @file     bids_brain_vision_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Contains the declaration of the BrainVisionReader class.
 *
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
