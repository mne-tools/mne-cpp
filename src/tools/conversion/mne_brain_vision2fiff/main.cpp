//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
 * @brief    Convert BrainVision EEG files (.vhdr/.vmrk/.eeg) to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// BrainVision header structures
//=============================================================================================================

struct BVChannelInfo {
    QString name;
    QString refName;
    double resolution;        // microvolts per bit
    QString unit;
};

struct BVMarker {
    QString type;
    QString description;
    int position;             // sample index
    int duration;             // in samples
    int channel;              // 0 = all channels
};

struct BVHeader {
    // Common Infos
    QString dataFile;
    QString markerFile;
    QString dataFormat;       // BINARY or ASCII
    int dataOrientation;      // 0 = MULTIPLEXED, 1 = VECTORIZED
    int numberOfChannels;
    double samplingInterval;  // in microseconds

    // Binary Format
    QString binaryFormat;     // INT_16, UINT_16, IEEE_FLOAT_32

    // Channel info
    QList<BVChannelInfo> channels;

    // Markers
    QList<BVMarker> markers;
};

//=============================================================================================================

static bool parseBVHeader(const QString &vhdrPath, BVHeader &hdr)
{
    QFile file(vhdrPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open header file: %s", qPrintable(vhdrPath));
        return false;
    }

    QTextStream in(&file);
    QString currentSection;
    int chIdx = 0;

    hdr.dataOrientation = 0;  // default: multiplexed
    hdr.numberOfChannels = 0;
    hdr.samplingInterval = 1000.0;  // 1 ms default
    hdr.binaryFormat = "INT_16";

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        // Skip empty lines and comments
        if (line.isEmpty() || line.startsWith(';'))
            continue;

        // Section header
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            chIdx = 0;
            continue;
        }

        // Key=value
        int eqPos = line.indexOf('=');
        if (eqPos < 0)
            continue;

        QString key = line.left(eqPos).trimmed();
        QString value = line.mid(eqPos + 1).trimmed();

        if (currentSection == "Common Infos") {
            if (key == "DataFile")
                hdr.dataFile = value;
            else if (key == "MarkerFile")
                hdr.markerFile = value;
            else if (key == "DataFormat")
                hdr.dataFormat = value;
            else if (key == "DataOrientation") {
                hdr.dataOrientation = (value.toUpper() == "VECTORIZED") ? 1 : 0;
            }
            else if (key == "NumberOfChannels")
                hdr.numberOfChannels = value.toInt();
            else if (key == "SamplingInterval")
                hdr.samplingInterval = value.toDouble();
        }
        else if (currentSection == "Binary Infos") {
            if (key == "BinaryFormat")
                hdr.binaryFormat = value;
        }
        else if (currentSection == "Channel Infos") {
            // Format: ChN=name,refName,resolution,unit
            QStringList parts = value.split(',');
            BVChannelInfo ch;
            ch.name = (parts.size() > 0) ? parts[0].trimmed() : QString("Ch%1").arg(chIdx + 1);
            ch.refName = (parts.size() > 1) ? parts[1].trimmed() : QString();
            ch.resolution = (parts.size() > 2) ? parts[2].trimmed().toDouble() : 1.0;
            ch.unit = (parts.size() > 3) ? parts[3].trimmed() : "µV";
            if (ch.resolution == 0.0)
                ch.resolution = 1.0;
            hdr.channels.append(ch);
            chIdx++;
        }
    }

    file.close();

    if (hdr.numberOfChannels == 0)
        hdr.numberOfChannels = hdr.channels.size();

    // Fill missing channel definitions
    while (hdr.channels.size() < hdr.numberOfChannels) {
        BVChannelInfo ch;
        ch.name = QString("Ch%1").arg(hdr.channels.size() + 1);
        ch.resolution = 1.0;
        ch.unit = "µV";
        hdr.channels.append(ch);
    }

    return true;
}

//=============================================================================================================
// Parse BrainVision .vmrk marker file
//=============================================================================================================

static bool parseBVMarkers(const QString &vmrkPath, BVHeader &hdr)
{
    QFile file(vmrkPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        printf("Warning: Cannot open marker file: %s\n", qPrintable(vmrkPath));
        return false;
    }

    QTextStream in(&file);
    QString currentSection;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(';'))
            continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.length() - 2);
            continue;
        }

        if (currentSection == "Marker Infos") {
            int eqPos = line.indexOf('=');
            if (eqPos < 0)
                continue;

            QString value = line.mid(eqPos + 1).trimmed();
            QStringList parts = value.split(',');
            if (parts.size() >= 4) {
                BVMarker mkr;
                mkr.type = parts[0].trimmed();
                mkr.description = parts[1].trimmed();
                mkr.position = parts[2].trimmed().toInt();
                mkr.duration = parts[3].trimmed().toInt();
                mkr.channel = (parts.size() > 4) ? parts[4].trimmed().toInt() : 0;
                hdr.markers.append(mkr);
            }
        }
    }

    file.close();
    return true;
}

//=============================================================================================================
// Read BrainVision binary data
//=============================================================================================================

static bool readBVData(const QString &dataPath, const BVHeader &hdr, MatrixXd &data)
{
    QFile file(dataPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open data file: %s", qPrintable(dataPath));
        return false;
    }

    int nChan = hdr.numberOfChannels;
    qint64 fileSize = file.size();
    int bytesPerSample;

    if (hdr.binaryFormat == "IEEE_FLOAT_32")
        bytesPerSample = 4;
    else // INT_16, UINT_16
        bytesPerSample = 2;

    qint64 nSamples;
    if (hdr.dataOrientation == 0) {
        // Multiplexed: samples are interleaved [ch0_s0, ch1_s0, ..., chN_s0, ch0_s1, ...]
        nSamples = fileSize / (nChan * bytesPerSample);
    } else {
        // Vectorized: all samples for ch0, then ch1, etc.
        nSamples = fileSize / (nChan * bytesPerSample);
    }

    printf("Data: %d channels x %lld samples (%s)\n", nChan, nSamples, qPrintable(hdr.binaryFormat));

    data.resize(nChan, nSamples);
    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::LittleEndian);
    ds.setFloatingPointPrecision(QDataStream::SinglePrecision);

    if (hdr.dataOrientation == 0) {
        // Multiplexed
        for (qint64 s = 0; s < nSamples; ++s) {
            for (int c = 0; c < nChan; ++c) {
                double val = 0.0;
                if (hdr.binaryFormat == "IEEE_FLOAT_32") {
                    float fval;
                    ds >> fval;
                    val = static_cast<double>(fval);
                } else if (hdr.binaryFormat == "INT_16") {
                    qint16 ival;
                    ds >> ival;
                    val = static_cast<double>(ival);
                } else { // UINT_16
                    quint16 uval;
                    ds >> uval;
                    val = static_cast<double>(uval);
                }
                data(c, s) = val * hdr.channels[c].resolution;
            }
        }
    } else {
        // Vectorized
        for (int c = 0; c < nChan; ++c) {
            for (qint64 s = 0; s < nSamples; ++s) {
                double val = 0.0;
                if (hdr.binaryFormat == "IEEE_FLOAT_32") {
                    float fval;
                    ds >> fval;
                    val = static_cast<double>(fval);
                } else if (hdr.binaryFormat == "INT_16") {
                    qint16 ival;
                    ds >> ival;
                    val = static_cast<double>(ival);
                } else {
                    quint16 uval;
                    ds >> uval;
                    val = static_cast<double>(uval);
                }
                data(c, s) = val * hdr.channels[c].resolution;
            }
        }
    }

    file.close();
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_brain_vision2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert BrainVision EEG files (.vhdr/.vmrk/.eeg) to FIFF format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption vhdrOpt("vhdr", "Input BrainVision header file (.vhdr).", "file");
    parser.addOption(vhdrOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString vhdrFile = parser.value(vhdrOpt);
    QString outFile = parser.value(outOpt);

    if (vhdrFile.isEmpty()) { qCritical("--vhdr is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Parse header
    BVHeader hdr;
    if (!parseBVHeader(vhdrFile, hdr))
        return 1;

    QFileInfo vhdrInfo(vhdrFile);
    QDir baseDir = vhdrInfo.absoluteDir();

    printf("BrainVision header: %d channels, sfreq=%.1f Hz, format=%s\n",
           hdr.numberOfChannels,
           1e6 / hdr.samplingInterval,
           qPrintable(hdr.binaryFormat));

    // Parse markers if available
    if (!hdr.markerFile.isEmpty()) {
        QString vmrkPath = baseDir.filePath(hdr.markerFile);
        parseBVMarkers(vmrkPath, hdr);
        printf("Markers: %lld events\n", static_cast<long long>(hdr.markers.size()));
    }

    // Read binary data
    QString dataPath = baseDir.filePath(hdr.dataFile);
    MatrixXd data;
    if (hdr.dataFormat.toUpper() == "BINARY") {
        if (!readBVData(dataPath, hdr, data))
            return 1;
    } else {
        qCritical("ASCII data format not supported.");
        return 1;
    }

    // Convert resolution from µV to V
    // BrainVision resolution is typically in µV, FIFF expects V
    data *= 1e-6;

    // Build FiffInfo
    double sfreq = 1e6 / hdr.samplingInterval;  // samplingInterval is in µs
    int nChan = hdr.numberOfChannels;

    FiffInfo info;
    info.sfreq = sfreq;
    info.nchan = nChan;
    info.meas_date[0] = static_cast<fiff_int_t>(QDateTime::currentDateTime().toSecsSinceEpoch());
    info.meas_date[1] = 0;

    for (int c = 0; c < nChan; ++c) {
        FiffChInfo ch;
        ch.ch_name = hdr.channels[c].name;
        ch.scanNo = c + 1;
        ch.logNo = c + 1;

        // Classify channel type from name
        QString nameUp = ch.ch_name.toUpper();
        if (nameUp.startsWith("EOG") || nameUp.contains("EOG")) {
            ch.kind = FIFFV_EOG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameUp.startsWith("ECG") || nameUp.contains("ECG")) {
            ch.kind = FIFFV_ECG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameUp.startsWith("EMG") || nameUp.contains("EMG")) {
            ch.kind = FIFFV_EMG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (nameUp.startsWith("STI") || nameUp.startsWith("TRIG")) {
            ch.kind = FIFFV_STIM_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else {
            ch.kind = FIFFV_EEG_CH;
            ch.unit = FIFF_UNIT_V;
            ch.chpos.coil_type = FIFFV_COIL_EEG;
        }

        ch.cal = 1.0;
        ch.range = 1.0;
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }

    // Write FIFF
    QFile outF(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(outF, info, cals);
    if (!stream) {
        qCritical("Cannot open output: %s", qPrintable(outFile));
        return 1;
    }

    // Write data in chunks
    int nSamples = data.cols();
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    printf("Written FIFF: %s (%d channels, %d samples, %.1f Hz)\n",
           qPrintable(outFile), nChan, nSamples, sfreq);

    return 0;
}
