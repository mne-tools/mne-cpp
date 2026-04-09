//=============================================================================================================
/**
 * @file     bids_edf_reader.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Simon Heinke <simon.heinke@tu-ilmenau.de>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    2.1.0
 * @date     March, 2019
 *
 * @section  LICENSE
 *
 * Copyright (C) 2019, Christoph Dinh, Simon Heinke and Matti Hamalainen. All rights reserved.
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
 * @brief    Contains the declaration of the EDFReader class.
 *           Refactored from the mne_edf2fiff tool into a reusable library reader.
 *
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
