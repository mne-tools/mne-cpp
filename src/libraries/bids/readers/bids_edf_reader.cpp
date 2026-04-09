//=============================================================================================================
/**
 * @file     bids_edf_reader.cpp
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
 * @brief    Definition of the EDFReader class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_edf_reader.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QIODevice>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// EDFChannelInfo
//=============================================================================================================

FiffChInfo EDFChannelInfo::toFiffChInfo() const
{
    FiffChInfo info;

    info.scanNo = channelNumber;
    info.logNo = channelNumber;

    QString sLabelUpper = label.toUpper();
    if(!isMeasurement) {
        info.kind = sLabelUpper.contains("STIM") ? FIFFV_STIM_CH : FIFFV_MISC_CH;
    } else {
        if(sLabelUpper.contains("ECOG"))
            info.kind = FIFFV_ECOG_CH;
        else if(sLabelUpper.contains("SEEG"))
            info.kind = FIFFV_SEEG_CH;
        else if(sLabelUpper.contains("EEG"))
            info.kind = FIFFV_EEG_CH;
        else if(sLabelUpper.contains("MEG"))
            info.kind = FIFFV_MEG_CH;
        else if(sLabelUpper.contains("ECG"))
            info.kind = FIFFV_ECG_CH;
        else if(sLabelUpper.contains("EOG"))
            info.kind = FIFFV_EOG_CH;
        else if(sLabelUpper.contains("EMG"))
            info.kind = FIFFV_EMG_CH;
        else
            info.kind = FIFFV_MISC_CH;
    }

    QString sUnitUpper = physicalDimension.toUpper();
    if(sUnitUpper.endsWith("V") || sUnitUpper.endsWith("VOLT")) {
        info.unit = FIFF_UNIT_V;
        if(sUnitUpper.startsWith("U") || sUnitUpper.startsWith("MICRO"))
            info.unit_mul = FIFF_UNITM_MU;
        else if(sUnitUpper.startsWith("M") || sUnitUpper.startsWith("MILLI"))
            info.unit_mul = FIFF_UNITM_M;
        else if(sUnitUpper.startsWith("N") || sUnitUpper.startsWith("NANO"))
            info.unit_mul = FIFF_UNITM_N;
        else
            info.unit_mul = FIFF_UNITM_NONE;
    } else {
        info.unit = FIFF_UNIT_NONE;
        info.unit_mul = FIFF_UNITM_NONE;
    }

    info.cal = 1.0f;
    info.range = 1.0f;
    info.ch_name = label;

    return info;
}

//=============================================================================================================
// EDFReader
//=============================================================================================================

EDFReader::EDFReader(float fScaleFactor)
    : m_fScaleFactor(fScaleFactor)
{
}

//=============================================================================================================

EDFReader::~EDFReader()
{
    if(m_file.isOpen()) {
        m_file.close();
    }
}

//=============================================================================================================

bool EDFReader::open(const QString& sFilePath)
{
    m_sFilePath = sFilePath;
    m_file.setFileName(sFilePath);

    if(!m_file.open(QIODevice::ReadOnly)) {
        qWarning() << "[EDFReader::open] Could not open file:" << sFilePath;
        return false;
    }

    parseHeader(&m_file);
    m_bIsOpen = true;
    return true;
}

//=============================================================================================================

void EDFReader::parseHeader(QIODevice* pDev)
{
    if(pDev->pos() != 0) {
        pDev->seek(0);
    }

    // General header fields
    m_sVersionNo = QString::fromLatin1(pDev->read(EDF_VERSION)).trimmed();
    m_sPatientId = QString::fromLatin1(pDev->read(LOCAL_PATIENT_INFO)).trimmed();
    m_sRecordingId = QString::fromLatin1(pDev->read(LOCAL_RECORD_INFO)).trimmed();
    m_startDateTime.setDate(QDate::fromString(QString::fromLatin1(pDev->read(STARTDATE)), "dd.MM.yy"));
    m_startDateTime = m_startDateTime.addYears(100);
    m_startDateTime.setTime(QTime::fromString(QString::fromLatin1(pDev->read(STARTTIME)), "hh.mm.ss"));
    m_iNumBytesInHeader = QString::fromLatin1(pDev->read(NUM_BYTES_HEADER)).toInt();
    pDev->read(HEADER_RESERVED);
    m_iNumDataRecords = QString::fromLatin1(pDev->read(NUM_DATA_RECORDS)).toInt();
    m_fDataRecordsDuration = QString::fromLatin1(pDev->read(DURATION_DATA_RECS)).toFloat();
    m_iNumChannels = QString::fromLatin1(pDev->read(NUM_SIGNALS)).toInt();

    // Per-channel fields (read in EDF-specified order: all labels, then all transducers, etc.)
    QVector<QString> vLabels, vTransducers, vPhysDims, vPrefilterings;
    QVector<float> vPhysMins, vPhysMaxs;
    QVector<long> vDigMins, vDigMaxs, vSamplesPerRecord;

    for(int i = 0; i < m_iNumChannels; ++i)
        vLabels.push_back(QString::fromLatin1(pDev->read(SIG_LABEL)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        vTransducers.push_back(QString::fromLatin1(pDev->read(SIG_TRANSDUCER)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        vPhysDims.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_DIM)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        vPhysMins.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_MIN)).toFloat());
    for(int i = 0; i < m_iNumChannels; ++i)
        vPhysMaxs.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_MAX)).toFloat());
    for(int i = 0; i < m_iNumChannels; ++i)
        vDigMins.push_back(QString::fromLatin1(pDev->read(SIG_DIG_MIN)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        vDigMaxs.push_back(QString::fromLatin1(pDev->read(SIG_DIG_MAX)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        vPrefilterings.push_back(QString::fromLatin1(pDev->read(SIG_PREFILTERING)).trimmed());
    for(int i = 0; i < m_iNumChannels; ++i)
        vSamplesPerRecord.push_back(QString::fromLatin1(pDev->read(SIG_NUM_SAMPLES)).toLong());
    for(int i = 0; i < m_iNumChannels; ++i)
        pDev->read(SIG_RESERVED);

    // Build channel info structs
    m_vAllChannels.clear();
    for(int i = 0; i < m_iNumChannels; ++i) {
        EDFChannelInfo ch;
        ch.channelNumber = i;
        ch.label = vLabels[i];
        ch.transducerType = vTransducers[i];
        ch.physicalDimension = vPhysDims[i];
        ch.prefiltering = vPrefilterings[i];
        ch.physicalMin = vPhysMins[i];
        ch.physicalMax = vPhysMaxs[i];
        ch.digitalMin = vDigMins[i];
        ch.digitalMax = vDigMaxs[i];
        ch.samplesPerRecord = vSamplesPerRecord[i];
        ch.sampleCount = vSamplesPerRecord[i] * m_iNumDataRecords;
        ch.frequency = (m_fDataRecordsDuration > 0.0f)
                         ? vSamplesPerRecord[i] / m_fDataRecordsDuration
                         : 0.0f;
        ch.isMeasurement = false;
        m_vAllChannels.push_back(ch);
    }

    // Verify header size consistency
    if(pDev->pos() != m_iNumBytesInHeader) {
        qWarning() << "[EDFReader::parseHeader] Header byte count mismatch: read"
                    << pDev->pos() << "expected" << m_iNumBytesInHeader;
    }

    // Calculate bytes per data record
    m_iNumBytesPerDataRecord = 0;
    for(const auto& ch : m_vAllChannels) {
        m_iNumBytesPerDataRecord += ch.samplesPerRecord * 2;  // 16-bit integers
    }

    // Identify measurement channels (those with the highest sample rate)
    long iMaxSamplesPerRecord = -1;
    for(const auto& ch : m_vAllChannels) {
        if(ch.samplesPerRecord > iMaxSamplesPerRecord) {
            iMaxSamplesPerRecord = ch.samplesPerRecord;
        }
    }

    m_vMeasChannels.clear();
    for(int i = 0; i < m_vAllChannels.size(); ++i) {
        if(m_vAllChannels[i].samplesPerRecord == iMaxSamplesPerRecord) {
            m_vAllChannels[i].isMeasurement = true;
            m_vMeasChannels.push_back(m_vAllChannels[i]);
        }
    }
}

//=============================================================================================================

FiffInfo EDFReader::getInfo() const
{
    FiffInfo info;
    info.nchan = m_vMeasChannels.size();

    for(const auto& ch : m_vMeasChannels) {
        FiffChInfo fiffCh = ch.toFiffChInfo();
        info.chs.append(fiffCh);
        info.ch_names.append(fiffCh.ch_name);
    }

    info.sfreq = getFrequency();
    return info;
}

//=============================================================================================================

MatrixXf EDFReader::readRawSegment(int iStartSampleIdx, int iEndSampleIdx) const
{
    if(!m_bIsOpen) {
        qWarning() << "[EDFReader::readRawSegment] File not open";
        return MatrixXf();
    }

    long totalSamples = getSampleCount();
    if(iStartSampleIdx < 0 || iStartSampleIdx >= totalSamples ||
       iEndSampleIdx < 0 || iEndSampleIdx > totalSamples) {
        qWarning() << "[EDFReader::readRawSegment] Index out of bounds:"
                    << iStartSampleIdx << "-" << iEndSampleIdx;
        return MatrixXf();
    }

    int iNumSamples = iEndSampleIdx - iStartSampleIdx;
    if(iNumSamples <= 0) {
        return MatrixXf();
    }

    int iSamplesPerRecord = m_vMeasChannels.isEmpty() ? 0 : m_vMeasChannels[0].samplesPerRecord;
    if(iSamplesPerRecord <= 0) {
        return MatrixXf();
    }

    // Calculate which data records to read
    int iFirstRecord = iStartSampleIdx / iSamplesPerRecord;
    int iRelativeFirst = iStartSampleIdx % iSamplesPerRecord;
    int iNumRecords = static_cast<int>(
        std::ceil(static_cast<float>(iNumSamples + iRelativeFirst) / iSamplesPerRecord));

    // Seek to first needed data record
    m_file.seek(m_iNumBytesInHeader + static_cast<qint64>(iFirstRecord) * m_iNumBytesPerDataRecord);

    // Read needed data records
    QVector<QByteArray> vRecords;
    vRecords.reserve(iNumRecords);
    for(int i = 0; i < iNumRecords; ++i) {
        vRecords.push_back(m_file.read(m_iNumBytesPerDataRecord));
    }

    // Demultiplex: channels are interleaved within each record
    QVector<QVector<int>> vRawPatches(m_vAllChannels.size());
    for(int iRec = 0; iRec < vRecords.size(); ++iRec) {
        int iOffset = 0;
        for(int iCh = 0; iCh < m_vAllChannels.size(); ++iCh) {
            int nSamp = m_vAllChannels[iCh].samplesPerRecord;
            QVector<int> patch(nSamp);
            for(int s = 0; s < nSamp; ++s) {
                int byteIdx = (iOffset + s) * 2;
                // 16-bit little-endian signed integer
                patch[s] = static_cast<int16_t>(
                    (static_cast<unsigned char>(vRecords[iRec].at(byteIdx + 1)) << 8) |
                    (static_cast<unsigned char>(vRecords[iRec].at(byteIdx))));
            }
            iOffset += nSamp;
            vRawPatches[iCh] += patch;
        }
    }

    // Filter to measurement channels only
    QVector<QVector<int>> vMeasPatches;
    vMeasPatches.reserve(m_vMeasChannels.size());
    for(int iCh = 0; iCh < m_vAllChannels.size(); ++iCh) {
        if(m_vAllChannels[iCh].isMeasurement) {
            vMeasPatches.push_back(vRawPatches[iCh]);
        }
    }

    // Scale and copy to result matrix
    MatrixXf result(vMeasPatches.size(), iNumSamples);

    for(int iCh = 0; iCh < vMeasPatches.size(); ++iCh) {
        const EDFChannelInfo& ch = m_vMeasChannels[iCh];
        float digRange = static_cast<float>(ch.digitalMax - ch.digitalMin);
        float physRange = ch.physicalMax - ch.physicalMin;

        for(int s = 0; s < iNumSamples; ++s) {
            int rawIdx = s + iRelativeFirst;
            float physVal = static_cast<float>(vMeasPatches[iCh][rawIdx] - ch.digitalMin) / digRange
                            * physRange + ch.physicalMin;
            if(ch.isMeasurement) {
                physVal /= m_fScaleFactor;
            }
            result(iCh, s) = physVal;
        }
    }

    return result;
}

//=============================================================================================================

long EDFReader::getSampleCount() const
{
    if(!m_vMeasChannels.isEmpty()) {
        return m_vMeasChannels[0].sampleCount;
    }
    return 0;
}

//=============================================================================================================

float EDFReader::getFrequency() const
{
    if(!m_vMeasChannels.isEmpty()) {
        return m_vMeasChannels[0].frequency;
    }
    return 0.0f;
}

//=============================================================================================================

int EDFReader::getChannelCount() const
{
    return m_vMeasChannels.size();
}

//=============================================================================================================

FiffRawData EDFReader::toFiffRawData() const
{
    FiffRawData raw;
    raw.info = getInfo();
    raw.first_samp = 0;
    raw.last_samp = getSampleCount();

    RowVectorXd cals(raw.info.nchan);
    for(int i = 0; i < raw.info.chs.size(); ++i) {
        cals[i] = static_cast<double>(raw.info.chs[i].cal);
    }
    raw.cals = cals;

    return raw;
}

//=============================================================================================================

QString EDFReader::formatName() const
{
    return QStringLiteral("EDF");
}

//=============================================================================================================

bool EDFReader::supportsExtension(const QString& sExtension) const
{
    QString ext = sExtension.toLower();
    return (ext == ".edf" || ext == ".bdf");
}

//=============================================================================================================

QVector<EDFChannelInfo> EDFReader::getAllChannelInfos() const
{
    return m_vAllChannels;
}

//=============================================================================================================

QVector<EDFChannelInfo> EDFReader::getMeasurementChannelInfos() const
{
    return m_vMeasChannels;
}
