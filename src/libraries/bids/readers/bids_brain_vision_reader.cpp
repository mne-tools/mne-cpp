//=============================================================================================================
/**
 * @file     bids_brain_vision_reader.cpp
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
 * @brief    Definition of the BrainVisionReader class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bids_brain_vision_reader.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QRegularExpression>
#include <QtEndian>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace BIDSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// BrainVisionChannelInfo
//=============================================================================================================

FiffChInfo BrainVisionChannelInfo::toFiffChInfo() const
{
    FiffChInfo info;
    info.scanNo = channelNumber;
    info.logNo = channelNumber;

    // Detect channel type from name
    QString nameUpper = name.toUpper();
    if(nameUpper.contains("ECOG"))
        info.kind = FIFFV_ECOG_CH;
    else if(nameUpper.contains("SEEG"))
        info.kind = FIFFV_SEEG_CH;
    else if(nameUpper.contains("EOG") || nameUpper == "HEOGL" || nameUpper == "HEOGR" || nameUpper == "VEOGB")
        info.kind = FIFFV_EOG_CH;
    else if(nameUpper.contains("ECG") || nameUpper.contains("EKG"))
        info.kind = FIFFV_ECG_CH;
    else if(nameUpper.contains("EMG"))
        info.kind = FIFFV_EMG_CH;
    else if(nameUpper == "STI 014" || nameUpper.contains("STIM"))
        info.kind = FIFFV_STIM_CH;
    else {
        // Check if unit suggests a voltage measurement (EEG)
        float scale = BrainVisionReader::unitScale(unit);
        if(scale > 0.0f)
            info.kind = FIFFV_EEG_CH;
        else
            info.kind = FIFFV_MISC_CH;
    }

    // Map unit
    QString unitUpper = unit.toUpper();
    if(unitUpper.contains("V") || unitUpper.isEmpty()) {
        info.unit = FIFF_UNIT_V;
        if(unitUpper.startsWith("N"))
            info.unit_mul = FIFF_UNITM_N;
        else if(unitUpper.startsWith(QStringLiteral("\u00B5")) || unitUpper.startsWith("U"))
            info.unit_mul = FIFF_UNITM_MU;
        else if(unitUpper.startsWith("M"))
            info.unit_mul = FIFF_UNITM_M;
        else
            info.unit_mul = FIFF_UNITM_NONE;
    } else {
        info.unit = FIFF_UNIT_NONE;
        info.unit_mul = FIFF_UNITM_NONE;
    }

    // Calibration: resolution is the scaling factor from raw → physical units
    info.cal = resolution;
    info.range = 1.0f;
    info.ch_name = name;

    return info;
}

//=============================================================================================================
// BrainVisionReader
//=============================================================================================================

BrainVisionReader::BrainVisionReader()
{
}

//=============================================================================================================

BrainVisionReader::~BrainVisionReader()
{
    if(m_dataFile.isOpen()) {
        m_dataFile.close();
    }
}

//=============================================================================================================

bool BrainVisionReader::open(const QString& sFilePath)
{
    m_sVhdrPath = sFilePath;

    if(!parseHeader(sFilePath)) {
        return false;
    }

    // Open the binary data file
    m_dataFile.setFileName(m_sDataPath);
    if(!m_dataFile.open(QIODevice::ReadOnly)) {
        qWarning() << "[BrainVisionReader::open] Could not open data file:" << m_sDataPath;
        return false;
    }

    computeSampleCount();

    // Parse markers if marker file exists
    if(!m_sMarkerPath.isEmpty()) {
        parseMarkers(m_sMarkerPath);
    }

    m_bIsOpen = true;
    return true;
}

//=============================================================================================================

bool BrainVisionReader::parseHeader(const QString& sVhdrPath)
{
    QFile hdrFile(sVhdrPath);
    if(!hdrFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BrainVisionReader::parseHeader] Could not open header:" << sVhdrPath;
        return false;
    }

    QFileInfo fi(sVhdrPath);
    QString sDir = fi.absolutePath();

    QTextStream in(&hdrFile);

    // Validate version line
    QString firstLine = in.readLine().trimmed();
    QRegularExpression versionRe(
        QStringLiteral("Brain ?Vision( Core| V-Amp)? Data( Exchange)? Header File,? Version [12]\\.0"),
        QRegularExpression::CaseInsensitiveOption);
    if(!versionRe.match(firstLine).hasMatch()) {
        qWarning() << "[BrainVisionReader::parseHeader] Unrecognized header version:" << firstLine;
        return false;
    }

    // Simple INI-style parser (sections + key=value)
    QString currentSection;
    QMap<QString, QMap<QString, QString>> sections;

    while(!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.isEmpty() || line.startsWith(';'))
            continue;

        if(line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.size() - 2);
            // Stop parsing at [Comment] section — it's free-form text
            if(currentSection.toLower() == "comment")
                break;
            continue;
        }

        int eqPos = line.indexOf('=');
        if(eqPos > 0 && !currentSection.isEmpty()) {
            QString key = line.left(eqPos).trimmed();
            QString value = line.mid(eqPos + 1).trimmed();
            sections[currentSection][key] = value;
        }
    }

    hdrFile.close();

    // Extract Common Infos — handle both "Common Infos" and "Common infos" (NeurOne variant)
    QMap<QString, QString> commonInfos;
    if(sections.contains("Common Infos"))
        commonInfos = sections["Common Infos"];
    else if(sections.contains("Common infos"))
        commonInfos = sections["Common infos"];

    if(commonInfos.isEmpty()) {
        qWarning() << "[BrainVisionReader::parseHeader] Missing [Common Infos] section";
        return false;
    }

    // Data file path (relative to .vhdr location)
    QString dataFileName = commonInfos.value("DataFile");
    if(dataFileName.isEmpty()) {
        qWarning() << "[BrainVisionReader::parseHeader] No DataFile specified";
        return false;
    }
    m_sDataPath = QDir(sDir).absoluteFilePath(dataFileName);

    // Marker file path
    QString markerFileName = commonInfos.value("MarkerFile");
    if(!markerFileName.isEmpty()) {
        m_sMarkerPath = QDir(sDir).absoluteFilePath(markerFileName);
    }

    // Data orientation (default: MULTIPLEXED)
    QString orientation = commonInfos.value("DataOrientation", "MULTIPLEXED").toUpper();
    m_orientation = (orientation == "VECTORIZED") ? BVOrientation::VECTORIZED : BVOrientation::MULTIPLEXED;

    // Number of channels
    m_iNumChannels = commonInfos.value("NumberOfChannels", "0").toInt();
    if(m_iNumChannels <= 0) {
        qWarning() << "[BrainVisionReader::parseHeader] Invalid channel count:" << m_iNumChannels;
        return false;
    }

    // Sampling interval in microseconds → frequency in Hz
    float samplingInterval = commonInfos.value("SamplingInterval", "0").toFloat();
    if(samplingInterval > 0.0f) {
        m_fSFreq = 1.0e6f / samplingInterval;
    }

    // Binary format
    QMap<QString, QString> binaryInfos;
    if(sections.contains("Binary Infos"))
        binaryInfos = sections["Binary Infos"];

    QString binaryFormat = binaryInfos.value("BinaryFormat", "INT_16").toUpper();
    if(binaryFormat == "INT_32")
        m_binaryFormat = BVBinaryFormat::INT_32;
    else if(binaryFormat == "IEEE_FLOAT_32")
        m_binaryFormat = BVBinaryFormat::IEEE_FLOAT_32;
    else
        m_binaryFormat = BVBinaryFormat::INT_16;

    // Channel Infos — Format: Ch<N>=<Name>,<Reference>,<Resolution>,<Unit>
    QMap<QString, QString> channelInfos;
    if(sections.contains("Channel Infos"))
        channelInfos = sections["Channel Infos"];

    m_vChannels.clear();
    m_vChannels.reserve(m_iNumChannels);

    for(int i = 1; i <= m_iNumChannels; ++i) {
        QString key = QStringLiteral("Ch%1").arg(i);
        QString value = channelInfos.value(key);

        BrainVisionChannelInfo ch;
        ch.channelNumber = i - 1;  // 0-based internally

        if(!value.isEmpty()) {
            // Decode \1 back to comma for name parsing
            QStringList parts = value.split(',');

            if(parts.size() >= 1)
                ch.name = parts[0].replace(QStringLiteral("\\1"), QStringLiteral(","));
            if(parts.size() >= 2)
                ch.reference = parts[1].replace(QStringLiteral("\\1"), QStringLiteral(","));
            if(parts.size() >= 3 && !parts[2].isEmpty())
                ch.resolution = parts[2].toFloat();
            if(parts.size() >= 4 && !parts[3].isEmpty())
                ch.unit = parts[3];
            else
                ch.unit = QStringLiteral("\u00B5V");  // Default: µV
        } else {
            ch.name = QStringLiteral("Ch%1").arg(i);
            ch.unit = QStringLiteral("\u00B5V");
        }

        m_vChannels.push_back(ch);
    }

    return true;
}

//=============================================================================================================

bool BrainVisionReader::parseMarkers(const QString& sVmrkPath)
{
    QFile mrkFile(sVmrkPath);
    if(!mrkFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[BrainVisionReader::parseMarkers] Could not open marker file:" << sVmrkPath;
        return false;
    }

    QTextStream in(&mrkFile);

    // Skip version line
    in.readLine();

    QString currentSection;
    m_vMarkers.clear();

    while(!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if(line.isEmpty() || line.startsWith(';'))
            continue;

        if(line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.size() - 2);
            continue;
        }

        if(currentSection == "Marker Infos") {
            // Format: Mk<N>=<Type>,<Description>,<Position>,<Duration>,<Channel>[,<Date>]
            int eqPos = line.indexOf('=');
            if(eqPos <= 0) continue;

            QString value = line.mid(eqPos + 1);
            QStringList parts = value.split(',');
            if(parts.size() < 5) continue;

            BrainVisionMarker marker;
            marker.type = parts[0].replace(QStringLiteral("\\1"), QStringLiteral(","));
            marker.description = parts[1].replace(QStringLiteral("\\1"), QStringLiteral(","));
            marker.position = parts[2].toLong() - 1;  // Convert 1-indexed to 0-indexed
            marker.duration = parts[3].toLong();
            marker.channel = parts[4].toInt();

            if(parts.size() >= 6 && !parts[5].isEmpty()) {
                // Date format: YYYYMMDDHHMMSSffffff (20 chars)
                QString dateStr = parts[5];
                if(dateStr.size() >= 14) {
                    marker.date = QDateTime(
                        QDate(dateStr.mid(0, 4).toInt(), dateStr.mid(4, 2).toInt(), dateStr.mid(6, 2).toInt()),
                        QTime(dateStr.mid(8, 2).toInt(), dateStr.mid(10, 2).toInt(), dateStr.mid(12, 2).toInt()));
                }
            }

            m_vMarkers.push_back(marker);
        }
    }

    mrkFile.close();
    return true;
}

//=============================================================================================================

void BrainVisionReader::computeSampleCount()
{
    if(m_iNumChannels <= 0) {
        m_lSampleCount = 0;
        return;
    }

    qint64 fileSize = m_dataFile.size();
    int bytesPerSample = 0;
    switch(m_binaryFormat) {
    case BVBinaryFormat::INT_16:        bytesPerSample = 2; break;
    case BVBinaryFormat::INT_32:        bytesPerSample = 4; break;
    case BVBinaryFormat::IEEE_FLOAT_32: bytesPerSample = 4; break;
    }

    m_lSampleCount = fileSize / (static_cast<qint64>(bytesPerSample) * m_iNumChannels);
}

//=============================================================================================================

float BrainVisionReader::unitScale(const QString& sUnit)
{
    QString u = sUnit.toLower();
    if(u == "v")                                    return 1.0f;
    if(u == "\u00B5v" || u == "uv" || u == "µv")   return 1.0e-6f;
    if(u == "mv")                                   return 1.0e-3f;
    if(u == "nv")                                   return 1.0e-9f;
    if(u == "c" || u == "\u00B0c")                  return 1.0f;  // temperature
    if(u == "\u00B5s" || u == "us")                 return 1.0e-6f;
    if(u == "s")                                    return 1.0f;
    if(u == "n/a")                                  return 1.0f;
    return 0.0f;  // unknown unit
}

//=============================================================================================================

FIFFLIB::FiffInfo BrainVisionReader::getInfo() const
{
    FiffInfo info;
    info.nchan = m_vChannels.size();
    info.sfreq = m_fSFreq;

    for(const auto& ch : m_vChannels) {
        FiffChInfo fiffCh = ch.toFiffChInfo();
        info.chs.append(fiffCh);
        info.ch_names.append(fiffCh.ch_name);
    }

    return info;
}

//=============================================================================================================

Eigen::MatrixXf BrainVisionReader::readRawSegment(int iStartSampleIdx, int iEndSampleIdx) const
{
    if(!m_bIsOpen) {
        qWarning() << "[BrainVisionReader::readRawSegment] File not open";
        return MatrixXf();
    }

    if(iStartSampleIdx < 0 || iStartSampleIdx >= m_lSampleCount ||
       iEndSampleIdx < 0 || iEndSampleIdx > m_lSampleCount ||
       iEndSampleIdx <= iStartSampleIdx) {
        qWarning() << "[BrainVisionReader::readRawSegment] Invalid range:"
                    << iStartSampleIdx << "-" << iEndSampleIdx;
        return MatrixXf();
    }

    int iNumSamples = iEndSampleIdx - iStartSampleIdx;
    int bytesPerValue = 0;
    switch(m_binaryFormat) {
    case BVBinaryFormat::INT_16:        bytesPerValue = 2; break;
    case BVBinaryFormat::INT_32:        bytesPerValue = 4; break;
    case BVBinaryFormat::IEEE_FLOAT_32: bytesPerValue = 4; break;
    }

    MatrixXf result(m_iNumChannels, iNumSamples);

    if(m_orientation == BVOrientation::MULTIPLEXED) {
        // Data layout: [ch1_t1, ch2_t1, ..., chN_t1, ch1_t2, ...]
        qint64 startByte = static_cast<qint64>(iStartSampleIdx) * m_iNumChannels * bytesPerValue;
        qint64 totalBytes = static_cast<qint64>(iNumSamples) * m_iNumChannels * bytesPerValue;
        m_dataFile.seek(startByte);

        QByteArray rawData = m_dataFile.read(totalBytes);
        const char* pData = rawData.constData();

        for(int s = 0; s < iNumSamples; ++s) {
            for(int ch = 0; ch < m_iNumChannels; ++ch) {
                qint64 offset = (static_cast<qint64>(s) * m_iNumChannels + ch) * bytesPerValue;
                float rawValue = 0.0f;

                switch(m_binaryFormat) {
                case BVBinaryFormat::INT_16:
                    rawValue = static_cast<float>(qFromLittleEndian<qint16>(pData + offset));
                    break;
                case BVBinaryFormat::INT_32:
                    rawValue = static_cast<float>(qFromLittleEndian<qint32>(pData + offset));
                    break;
                case BVBinaryFormat::IEEE_FLOAT_32:
                    rawValue = qFromLittleEndian<float>(pData + offset);
                    break;
                }

                // Apply channel resolution (cal) and unit scaling
                float cal = m_vChannels[ch].resolution;
                float scale = unitScale(m_vChannels[ch].unit);
                result(ch, s) = rawValue * cal * scale;
            }
        }
    } else {
        // VECTORIZED: Each channel contiguous
        // Layout: [ch1_t1, ch1_t2, ..., ch1_tM, ch2_t1, ...]
        for(int ch = 0; ch < m_iNumChannels; ++ch) {
            qint64 channelOffset = static_cast<qint64>(ch) * m_lSampleCount * bytesPerValue;
            qint64 startByte = channelOffset + static_cast<qint64>(iStartSampleIdx) * bytesPerValue;
            qint64 readBytes = static_cast<qint64>(iNumSamples) * bytesPerValue;

            m_dataFile.seek(startByte);
            QByteArray rawData = m_dataFile.read(readBytes);
            const char* pData = rawData.constData();

            float cal = m_vChannels[ch].resolution;
            float scale = unitScale(m_vChannels[ch].unit);

            for(int s = 0; s < iNumSamples; ++s) {
                qint64 offset = static_cast<qint64>(s) * bytesPerValue;
                float rawValue = 0.0f;

                switch(m_binaryFormat) {
                case BVBinaryFormat::INT_16:
                    rawValue = static_cast<float>(qFromLittleEndian<qint16>(pData + offset));
                    break;
                case BVBinaryFormat::INT_32:
                    rawValue = static_cast<float>(qFromLittleEndian<qint32>(pData + offset));
                    break;
                case BVBinaryFormat::IEEE_FLOAT_32:
                    rawValue = qFromLittleEndian<float>(pData + offset);
                    break;
                }

                result(ch, s) = rawValue * cal * scale;
            }
        }
    }

    return result;
}

//=============================================================================================================

long BrainVisionReader::getSampleCount() const
{
    return m_lSampleCount;
}

//=============================================================================================================

float BrainVisionReader::getFrequency() const
{
    return m_fSFreq;
}

//=============================================================================================================

int BrainVisionReader::getChannelCount() const
{
    return m_iNumChannels;
}

//=============================================================================================================

FiffRawData BrainVisionReader::toFiffRawData() const
{
    FiffRawData raw;
    raw.info = getInfo();
    raw.first_samp = 0;
    raw.last_samp = m_lSampleCount;

    RowVectorXd cals(raw.info.nchan);
    for(int i = 0; i < raw.info.chs.size(); ++i) {
        cals[i] = static_cast<double>(raw.info.chs[i].cal);
    }
    raw.cals = cals;

    return raw;
}

//=============================================================================================================

QString BrainVisionReader::formatName() const
{
    return QStringLiteral("BrainVision");
}

//=============================================================================================================

bool BrainVisionReader::supportsExtension(const QString& sExtension) const
{
    QString ext = sExtension.toLower();
    return (ext == ".vhdr" || ext == ".ahdr");
}

//=============================================================================================================

QVector<BrainVisionMarker> BrainVisionReader::getMarkers() const
{
    return m_vMarkers;
}

//=============================================================================================================

QVector<BrainVisionChannelInfo> BrainVisionReader::getChannelInfos() const
{
    return m_vChannels;
}
