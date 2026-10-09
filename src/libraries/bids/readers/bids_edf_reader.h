//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_edf_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    @ref BIDSLIB::AbstractFormatReader implementation for European Data Format (EDF / EDF+) and BioSemi BDF files.
 *
 * EDF stores a recording as a fixed-size ASCII header followed by a
 * stream of @c duration_seconds-long "data records"; inside each
 * record the channels appear in order, with each channel contributing
 * @c samples_per_record little-endian @c int16 samples (24-bit integers
 * in a @c .bdf file). The header
 * additionally lists per-channel physical / digital min / max which
 * @ref BIDSLIB::EDFReader uses to derive the affine calibration that
 * converts the on-disk integers back into the channel's physical unit
 * (volts, microvolts, degrees Celsius, …).
 *
 * The same parser feeds @ref BIDSLIB::BidsRawData and the @c mne_edf2fiff
 * converter. Each channel is scaled from its physical dimension
 * (µV, mV) into SI units, as in mne read_raw_edf, so voltages emerge
 * in volts, matching the MNE-CPP @c FIFFLIB convention. A "Status" or
 * "Trigger" channel keeps the low 17 bits of its value (raw for BDF),
 * as in mne read_raw_edf / read_raw_bdf.
 *
 * Format reference: Kemp & Olivan, "European data format 'plus'
 * (EDF+)", Clin. Neurophysiol. 114 (2003) 1755–1761; spec at
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
 *
 * @snippet ex_bids/main.cpp edf_channel_info
 */
struct BIDSSHARED_EXPORT EDFChannelInfo
{
    int channelNumber{-1};
    QString label;
    QString transducerType;
    QString physicalDimension;
    QString prefiltering;
    float physicalMin{0.0f};
    float physicalMax{0.0f};
    long digitalMin{0};
    long digitalMax{0};
    long samplesPerRecord{0};
    long sampleCount{0};
    float frequency{0.0f};
    bool isMeasurement{false};

    FIFFLIB::FiffChInfo toFiffChInfo() const;

    /** @return Factor from the physical dimension to SI (µV/uV 1e-6, mV 1e-3, otherwise 1; 1 for a "Status"/"Trigger" stim channel), as in mne read_raw_edf. */
    float toSi() const;
};

//=============================================================================================================
/**
 * @brief The EDFReader reads European Data Format (EDF/EDF+) files and exposes them through
 *        the AbstractFormatReader interface.
 *
 *        EDF stores data as 16-bit (BDF: 24-bit) little-endian integers in fixed-duration
 *        "data records", with channels interleaved within each record.
 *
 * @snippet ex_bids/main.cpp edf_reader_usage
 */
class BIDSSHARED_EXPORT EDFReader : public AbstractFormatReader
{
public:
    //=========================================================================================================
    /**
     * @brief EDFReader Default constructor.
     */
    EDFReader();

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
     *
     * @return Infos for every signal in the EDF header, including annotation/extra channels.
     */
    QVector<EDFChannelInfo> getAllChannelInfos() const;

    //=========================================================================================================
    /**
     * @brief Return measurement channel infos only.
     *
     * @return Infos for the measurement channels only, in channel order.
     */
    QVector<EDFChannelInfo> getMeasurementChannelInfos() const;

private:
    // EDF header field byte lengths
    enum EDFHeaderFieldLengths
    {
        EDF_VERSION = 8,
        LOCAL_PATIENT_INFO = 80,
        LOCAL_RECORD_INFO = 80,
        STARTDATE = 8,
        STARTTIME = 8,
        NUM_BYTES_HEADER = 8,
        HEADER_RESERVED = 44,
        NUM_DATA_RECORDS = 8,
        DURATION_DATA_RECS = 8,
        NUM_SIGNALS = 4,
        SIG_LABEL = 16,
        SIG_TRANSDUCER = 80,
        SIG_PHYS_DIM = 8,
        SIG_PHYS_MIN = 8,
        SIG_PHYS_MAX = 8,
        SIG_DIG_MIN = 8,
        SIG_DIG_MAX = 8,
        SIG_PREFILTERING = 80,
        SIG_NUM_SAMPLES = 8,
        SIG_RESERVED = 32,
    };

    void parseHeader(QIODevice* pDev);

    QString m_sFilePath;

    // Header data
    QString m_sVersionNo;
    QString m_sPatientId;
    QString m_sRecordingId;
    QDateTime m_startDateTime;
    int m_iNumBytesInHeader{0};
    int m_iNumDataRecords{0};
    float m_fDataRecordsDuration{0.0f};
    int m_iNumChannels{0};
    int m_iNumBytesPerDataRecord{0};

    QVector<EDFChannelInfo> m_vAllChannels;
    QVector<EDFChannelInfo> m_vMeasChannels;

    mutable QFile m_file;
    bool m_bIsOpen{false};
    int m_iBytesPerSample{2}; // 3 for BDF
};

} // namespace BIDSLIB

#endif // BIDS_EDF_READER_H
