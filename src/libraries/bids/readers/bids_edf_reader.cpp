//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     bids_edf_reader.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Implementation of @ref BIDSLIB::EDFReader — European Data Format (EDF / EDF+) parser behind @ref BIDSLIB::AbstractFormatReader.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_edf_reader.h"
#include <base/log.h>
#include <base/string_utils.h>

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QFileInfo>
#include <QIODevice>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <algorithm>
#include <array>
#include <string>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE LOCAL FUNCTIONS
//=============================================================================================================

namespace
{

/** Splits "a.b.c" (dots or dashes) into three fields; empty when the text has another shape. */
std::optional<std::array<std::string, 3>> splitTriple(std::string_view text, char separator)
{
    std::array<std::string, 3> fields;
    std::size_t start = 0;
    for (std::size_t k = 0; k < 3; ++k) {
        const std::size_t end = k < 2 ? text.find(separator, start) : text.size();
        if (end == std::string_view::npos) {
            return std::nullopt;
        }
        fields[k] = std::string(BASELIB::trimmed(text.substr(start, end - start)));
        start = end + 1;
    }
    return fields;
}

/** A calendar date from day, month and year, or empty when it does not exist. */
std::optional<std::chrono::sys_days> civilDate(std::optional<int> day, std::optional<int> month, std::optional<int> year)
{
    if (!day || !month || !year) {
        return std::nullopt;
    }
    const std::chrono::year_month_day date{std::chrono::year{*year}, std::chrono::month(static_cast<unsigned>(*month)),
                                           std::chrono::day(static_cast<unsigned>(*day))};
    return date.ok() ? std::optional(std::chrono::sys_days{date}) : std::nullopt;
}

} // namespace

//=============================================================================================================

std::optional<std::chrono::sys_time<std::chrono::microseconds>> EDFReader::parseStartDate(std::string_view recordingId,
                                                                                          std::string_view startDate,
                                                                                          std::string_view startTime)
{
    using namespace std::chrono;
    // EDF+ "Startdate dd-MMM-yyyy ..." carries all four digits of the year and wins over the header date
    std::optional<sys_days> date;
    const std::vector<std::string> recording = BASELIB::split(BASELIB::trimmed(recordingId), ' ');
    if (recording.size() == 5) {
        if (const auto fields = splitTriple(recording[1], '-')) {
            static constexpr std::array<std::string_view, 12> months{"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                                                     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
            const auto month = std::ranges::find_if(months, [&](std::string_view name) { return BASELIB::equalsIgnoreCase(name, (*fields)[1]); });
            if (month != months.end()) {
                date = civilDate(BASELIB::toInt((*fields)[0]), static_cast<int>(month - months.begin()) + 1, BASELIB::toInt((*fields)[2]));
            }
        }
    }
    // Otherwise the two-digit header year: 85-99 is 1985-1999, 00-84 is 2000-2084
    if (!date) {
        if (const auto fields = splitTriple(startDate, '.')) {
            std::optional<int> year = BASELIB::toInt((*fields)[2]);
            if (year) {
                *year += *year < 85 ? 2000 : 1900;
            }
            date = civilDate(BASELIB::toInt((*fields)[0]), BASELIB::toInt((*fields)[1]), year);
        }
    }
    if (!date) {
        return std::nullopt;
    }
    sys_time<microseconds> time = *date;
    if (const auto fields = splitTriple(startTime, '.')) {
        const auto hour = BASELIB::toInt((*fields)[0]);
        const auto minute = BASELIB::toInt((*fields)[1]);
        const auto second = BASELIB::toInt((*fields)[2]);
        if (hour && minute && second) {
            time += hours(*hour) + minutes(*minute) + seconds(*second);
        }
    }
    return time;
}

//=============================================================================================================
// EDFChannelInfo
//=============================================================================================================

namespace
{

// mne read_raw_edf stim_channel="auto"
bool isAutoStimLabel(const QString& label)
{
    return label.compare(QLatin1String("status"), Qt::CaseInsensitive) == 0 || label.compare(QLatin1String("trigger"), Qt::CaseInsensitive) == 0;
}

} // namespace

//=============================================================================================================

FiffChInfo EDFChannelInfo::toFiffChInfo() const
{
    FiffChInfo info;

    info.scanNo = channelNumber;
    info.logNo = channelNumber;

    QString sLabelUpper = label.toUpper();
    if (!isMeasurement) {
        info.kind = sLabelUpper.contains("STIM") ? FIFFV_STIM_CH : FIFFV_MISC_CH;
    } else if (isAutoStimLabel(label)) {
        info.kind = FIFFV_STIM_CH;
    } else {
        if (sLabelUpper.contains("ECOG"))
            info.kind = FIFFV_ECOG_CH;
        else if (sLabelUpper.contains("SEEG"))
            info.kind = FIFFV_SEEG_CH;
        else if (sLabelUpper.contains("EEG"))
            info.kind = FIFFV_EEG_CH;
        else if (sLabelUpper.contains("MEG"))
            info.kind = FIFFV_MEG_CH;
        else if (sLabelUpper.contains("ECG"))
            info.kind = FIFFV_ECG_CH;
        else if (sLabelUpper.contains("EOG"))
            info.kind = FIFFV_EOG_CH;
        else if (sLabelUpper.contains("EMG"))
            info.kind = FIFFV_EMG_CH;
        else
            info.kind = FIFFV_EEG_CH;
    }

    // Samples are converted to SI by toSi(), so voltages are stored in V
    info.unit = (info.kind != FIFFV_STIM_CH && physicalDimension.endsWith(QLatin1Char('V'))) ? FIFF_UNIT_V : FIFF_UNIT_NONE;
    info.unit_mul = FIFF_UNITM_NONE;

    info.cal = 1.0f;
    info.range = 1.0f;
    info.ch_name = label.toStdString();

    return info;
}

//=============================================================================================================
// EDFReader
//=============================================================================================================

float EDFChannelInfo::toSi() const
{
    // mne read_raw_edf: µV (micro sign, Greek mu, Shift-JIS mu) and uV are 1e-6, mV 1e-3, anything else 1
    if (isAutoStimLabel(label))
        return 1.0f;
    const QString& d = physicalDimension;
    if (d == QStringLiteral("\u03BCV") || d == QStringLiteral("\u00B5V") || d == QLatin1String("\x83\xCAV") || d == QLatin1String("uV"))
        return 1.0e-6f;
    if (d == QLatin1String("mV"))
        return 1.0e-3f;
    return 1.0f;
}

//=============================================================================================================

EDFReader::EDFReader()
{
}

//=============================================================================================================

EDFReader::~EDFReader()
{
    if (m_file.isOpen()) {
        m_file.close();
    }
}

//=============================================================================================================

bool EDFReader::open(const QString& sFilePath)
{
    m_sFilePath = sFilePath;
    m_file.setFileName(sFilePath);

    if (!m_file.open(QIODevice::ReadOnly)) {
        BASELIB::Log::warning("[EDFReader::open] Could not open file: {}", sFilePath.toStdString());
        return false;
    }

    // Like mne, the extension decides between EDF (16 bit) and BDF (24 bit)
    m_iBytesPerSample = QFileInfo(sFilePath).suffix().compare(QLatin1String("bdf"), Qt::CaseInsensitive) == 0 ? 3 : 2;
    parseHeader(&m_file);
    m_bIsOpen = true;
    return true;
}

//=============================================================================================================

void EDFReader::parseHeader(QIODevice* pDev)
{
    if (pDev->pos() != 0) {
        pDev->seek(0);
    }

    // General header fields
    m_sVersionNo = QString::fromLatin1(pDev->read(EDF_VERSION)).trimmed();
    m_sPatientId = QString::fromLatin1(pDev->read(LOCAL_PATIENT_INFO)).trimmed();
    m_sRecordingId = QString::fromLatin1(pDev->read(LOCAL_RECORD_INFO)).trimmed();
    const std::string startDate = pDev->read(STARTDATE).toStdString();
    const std::string startTime = pDev->read(STARTTIME).toStdString();
    m_measDate = parseStartDate(m_sRecordingId.toStdString(), startDate, startTime);
    m_iNumBytesInHeader = QString::fromLatin1(pDev->read(NUM_BYTES_HEADER)).trimmed().toInt();
    pDev->read(HEADER_RESERVED);
    m_iNumDataRecords = QString::fromLatin1(pDev->read(NUM_DATA_RECORDS)).trimmed().toInt();
    m_fDataRecordsDuration = QString::fromLatin1(pDev->read(DURATION_DATA_RECS)).trimmed().toFloat();
    m_iNumChannels = QString::fromLatin1(pDev->read(NUM_SIGNALS)).trimmed().toInt();

    // Per-channel fields (read in EDF-specified order: all labels, then all transducers, etc.)
    QVector<QString> vLabels, vTransducers, vPhysDims, vPrefilterings;
    QVector<float> vPhysMins, vPhysMaxs;
    QVector<long> vDigMins, vDigMaxs, vSamplesPerRecord;

    for (int i = 0; i < m_iNumChannels; ++i)
        vLabels.push_back(QString::fromLatin1(pDev->read(SIG_LABEL)).trimmed());
    for (int i = 0; i < m_iNumChannels; ++i)
        vTransducers.push_back(QString::fromLatin1(pDev->read(SIG_TRANSDUCER)).trimmed());
    for (int i = 0; i < m_iNumChannels; ++i)
        vPhysDims.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_DIM)).trimmed());
    for (int i = 0; i < m_iNumChannels; ++i)
        vPhysMins.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_MIN)).trimmed().toFloat());
    for (int i = 0; i < m_iNumChannels; ++i)
        vPhysMaxs.push_back(QString::fromLatin1(pDev->read(SIG_PHYS_MAX)).trimmed().toFloat());
    for (int i = 0; i < m_iNumChannels; ++i)
        vDigMins.push_back(QString::fromLatin1(pDev->read(SIG_DIG_MIN)).trimmed().toLong());
    for (int i = 0; i < m_iNumChannels; ++i)
        vDigMaxs.push_back(QString::fromLatin1(pDev->read(SIG_DIG_MAX)).trimmed().toLong());
    for (int i = 0; i < m_iNumChannels; ++i)
        vPrefilterings.push_back(QString::fromLatin1(pDev->read(SIG_PREFILTERING)).trimmed());
    for (int i = 0; i < m_iNumChannels; ++i)
        vSamplesPerRecord.push_back(QString::fromLatin1(pDev->read(SIG_NUM_SAMPLES)).trimmed().toLong());
    for (int i = 0; i < m_iNumChannels; ++i)
        pDev->read(SIG_RESERVED);

    // Build channel info structs
    m_vAllChannels.clear();
    for (int i = 0; i < m_iNumChannels; ++i) {
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
    if (pDev->pos() != m_iNumBytesInHeader) {
        BASELIB::Log::warning("[EDFReader::parseHeader] Header byte count mismatch: read {} expected {}", pDev->pos(), m_iNumBytesInHeader);
    }

    // Calculate bytes per data record
    m_iNumBytesPerDataRecord = 0;
    for (const auto& ch : m_vAllChannels) {
        m_iNumBytesPerDataRecord += ch.samplesPerRecord * m_iBytesPerSample;
    }

    // Identify measurement channels (those with the highest sample rate)
    long iMaxSamplesPerRecord = -1;
    for (const auto& ch : m_vAllChannels) {
        if (ch.samplesPerRecord > iMaxSamplesPerRecord) {
            iMaxSamplesPerRecord = ch.samplesPerRecord;
        }
    }

    m_vMeasChannels.clear();
    for (int i = 0; i < m_vAllChannels.size(); ++i) {
        if (m_vAllChannels[i].samplesPerRecord == iMaxSamplesPerRecord) {
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

    for (const auto& ch : m_vMeasChannels) {
        FiffChInfo fiffCh = ch.toFiffChInfo();
        info.chs.push_back(fiffCh);
        info.ch_names.push_back(fiffCh.ch_name);
    }

    info.sfreq = getFrequency();
    if (m_measDate) {
        info.set_meas_date(*m_measDate);
    }
    return info;
}

//=============================================================================================================

MatrixXf EDFReader::readRawSegment(int iStartSampleIdx, int iEndSampleIdx) const
{
    if (!m_bIsOpen) {
        BASELIB::Log::warning("[EDFReader::readRawSegment] File not open");
        return MatrixXf();
    }

    long totalSamples = getSampleCount();
    if (iStartSampleIdx < 0 || iStartSampleIdx >= totalSamples ||
        iEndSampleIdx < 0 || iEndSampleIdx > totalSamples) {
        BASELIB::Log::warning("[EDFReader::readRawSegment] Index out of bounds: {} - {}", iStartSampleIdx, iEndSampleIdx);
        return MatrixXf();
    }

    int iNumSamples = iEndSampleIdx - iStartSampleIdx;
    if (iNumSamples <= 0) {
        return MatrixXf();
    }

    int iSamplesPerRecord = m_vMeasChannels.isEmpty() ? 0 : m_vMeasChannels[0].samplesPerRecord;
    if (iSamplesPerRecord <= 0) {
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
    for (int i = 0; i < iNumRecords; ++i) {
        vRecords.push_back(m_file.read(m_iNumBytesPerDataRecord));
    }

    // Demultiplex: channels are interleaved within each record
    QVector<QVector<int>> vRawPatches(m_vAllChannels.size());
    for (int iRec = 0; iRec < vRecords.size(); ++iRec) {
        int iOffset = 0;
        for (int iCh = 0; iCh < m_vAllChannels.size(); ++iCh) {
            int nSamp = m_vAllChannels[iCh].samplesPerRecord;
            QVector<int> patch(nSamp);
            for (int s = 0; s < nSamp; ++s) {
                // Little-endian signed integers: 16 bit (EDF) or 24 bit (BDF)
                const auto* bytes = reinterpret_cast<const unsigned char*>(vRecords[iRec].constData()) + (iOffset + s) * m_iBytesPerSample;
                int value = bytes[0] | (bytes[1] << 8);
                if (m_iBytesPerSample == 3) {
                    value |= bytes[2] << 16;
                    patch[s] = (value & 0x800000) ? value - 0x1000000 : value;
                } else {
                    patch[s] = static_cast<int16_t>(value);
                }
            }
            iOffset += nSamp;
            vRawPatches[iCh] += patch;
        }
    }

    // Filter to measurement channels only
    QVector<QVector<int>> vMeasPatches;
    vMeasPatches.reserve(m_vMeasChannels.size());
    for (int iCh = 0; iCh < m_vAllChannels.size(); ++iCh) {
        if (m_vAllChannels[iCh].isMeasurement) {
            vMeasPatches.push_back(vRawPatches[iCh]);
        }
    }

    // Scale and copy to result matrix
    MatrixXf result(vMeasPatches.size(), iNumSamples);

    for (int iCh = 0; iCh < vMeasPatches.size(); ++iCh) {
        const EDFChannelInfo& ch = m_vMeasChannels[iCh];
        const double digRange = static_cast<double>(ch.digitalMax - ch.digitalMin);
        const double physRange = static_cast<double>(ch.physicalMax) - ch.physicalMin;
        const bool stim = isAutoStimLabel(ch.label);

        for (int s = 0; s < iNumSamples; ++s) {
            const int raw = vMeasPatches[iCh][s + iRelativeFirst];
            // mne read_raw_bdf keeps the raw stim value; both readers keep its low 17 bits
            if (stim) {
                const double value = (m_iBytesPerSample == 3) ? raw : (raw - ch.digitalMin) / digRange * physRange + ch.physicalMin;
                result(iCh, s) = static_cast<float>(static_cast<int>(value) & 0x1FFFF);
            } else {
                result(iCh, s) = static_cast<float>(((raw - ch.digitalMin) / digRange * physRange + ch.physicalMin) * ch.toSi());
            }
        }
    }

    return result;
}

//=============================================================================================================

long EDFReader::getSampleCount() const
{
    if (!m_vMeasChannels.isEmpty()) {
        return m_vMeasChannels[0].sampleCount;
    }
    return 0;
}

//=============================================================================================================

float EDFReader::getFrequency() const
{
    if (!m_vMeasChannels.isEmpty()) {
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
    raw.last_samp = static_cast<int>(getSampleCount()) - 1;

    RowVectorXd cals(raw.info.nchan);
    for (int i = 0; i < std::ssize(raw.info.chs); ++i) {
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
