//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file bids_edf_reader.h
 * @since 2026
 * @date  April 2026
 * @brief @ref BIDSLIB::AbstractFormatReader implementation for European Data Format (EDF / EDF+) files.
 *
 * EDF stores a recording as a fixed-size ASCII header followed by a
 * stream of @c duration_seconds-long ``data records''; inside each
 * record the channels appear in order, with each channel contributing
 * @c samples_per_record little-endian @c int16 samples. The header
 * additionally lists per-channel physical / digital min / max which
 * @ref BIDSLIB::EDFReader uses to derive the affine calibration that
 * converts the on-disk integers back into the channel's physical unit
 * (volts, microvolts, degrees Celsius, …).
 *
 * The reader is the BIDS counterpart of the legacy @c mne_edf2fiff
 * command-line tool, refactored into a reusable library class so the
 * same parser feeds both @ref BIDSLIB::BidsRawData and standalone
 * converters. The optional @c fScaleFactor constructor argument
 * defaults to @c 1e6 so the canonical microvolt-stored EEG /
 * iEEG channels emerge in volts, matching the MNE-CPP @c FIFFLIB
 * convention.
 *
 * Format reference: Kemp & Olivan, ``European data format 'plus'
 * (EDF+)'', Clin. Neurophysiol. 114 (2003) 1755–1761; spec at
 * https://www.edfplus.info/specs/edf.html.
 */

#ifndef BIDS_EDF_READER_H
#define BIDS_EDF_READER_H

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

//=============================================================================================================
// DEFINE NAMESPACE BIDSLIB
//=============================================================================================================

namespace BIDSLIB
{

//=============================================================================================================
/**
 * @brief Channel-level metadata from the EDF header.
 */
struct BIDSSHARED_EXPORT EDFChannelInfo
{
    int     channelNumber{-1};
    QString label;
    QString transducerType;
    QString physicalDimension;
    QString prefiltering;
    float   physicalMin{0.0f};
    float   physicalMax{0.0f};
    long    digitalMin{0};
    long    digitalMax{0};
    long    samplesPerRecord{0};
    long    sampleCount{0};
    float   frequency{0.0f};
    bool    isMeasurement{false};

    FIFFLIB::FiffChInfo toFiffChInfo() const;
};

//=============================================================================================================
/**
 * @brief The EDFReader reads European Data Format (EDF/EDF+) files and exposes them through
 *        the AbstractFormatReader interface.
 *
 *        The EDF specification stores data as 16-bit little-endian integers in fixed-duration
 *        "data records", with channels interleaved within each record.
 */
class BIDSSHARED_EXPORT EDFReader : public AbstractFormatReader
{
public:
    //=========================================================================================================
    /**
     * @brief EDFReader Default constructor.
     * @param[in] fScaleFactor  Raw value scaling factor (default: 1e6 for uV→V conversion).
     */
    explicit EDFReader(float fScaleFactor = 1e6);

    ~EDFReader() override;

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
     * @brief Return all channel infos (measurement + extra).
     */
    QVector<EDFChannelInfo> getAllChannelInfos() const;

    //=========================================================================================================
    /**
     * @brief Return measurement channel infos only.
     */
    QVector<EDFChannelInfo> getMeasurementChannelInfos() const;

private:
    // EDF header field byte lengths
    enum EDFHeaderFieldLengths {
        EDF_VERSION         = 8,
        LOCAL_PATIENT_INFO  = 80,
        LOCAL_RECORD_INFO   = 80,
        STARTDATE           = 8,
        STARTTIME           = 8,
        NUM_BYTES_HEADER    = 8,
        HEADER_RESERVED     = 44,
        NUM_DATA_RECORDS    = 8,
        DURATION_DATA_RECS  = 8,
        NUM_SIGNALS         = 4,
        SIG_LABEL           = 16,
        SIG_TRANSDUCER      = 80,
        SIG_PHYS_DIM        = 8,
        SIG_PHYS_MIN        = 8,
        SIG_PHYS_MAX        = 8,
        SIG_DIG_MIN         = 8,
        SIG_DIG_MAX         = 8,
        SIG_PREFILTERING    = 80,
        SIG_NUM_SAMPLES     = 8,
        SIG_RESERVED        = 32,
    };

    void parseHeader(QIODevice* pDev);

    float   m_fScaleFactor;
    QString m_sFilePath;

    // Header data
    QString     m_sVersionNo;
    QString     m_sPatientId;
    QString     m_sRecordingId;
    QDateTime   m_startDateTime;
    int         m_iNumBytesInHeader{0};
    int         m_iNumDataRecords{0};
    float       m_fDataRecordsDuration{0.0f};
    int         m_iNumChannels{0};
    int         m_iNumBytesPerDataRecord{0};

    QVector<EDFChannelInfo> m_vAllChannels;
    QVector<EDFChannelInfo> m_vMeasChannels;

    mutable QFile m_file;
    bool m_bIsOpen{false};
};

} // namespace BIDSLIB

#endif // BIDS_EDF_READER_H
