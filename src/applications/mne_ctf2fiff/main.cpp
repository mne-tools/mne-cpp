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
 * @brief    Convert CTF .ds MEG datasets to FIFF format.
 *
 *           Reads the CTF .ds directory structure (res4 header + meg4 data)
 *           and writes a standard FIFF raw file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_types.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QDataStream>

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

#define PROGRAM_VERSION "2.0.0"

// CTF sensor types (from res4 file)
#define CTF_SEN_TYPE_MEG        5     // MEG magnetometer/gradiometer
#define CTF_SEN_TYPE_REF_MAG    0     // Reference magnetometer
#define CTF_SEN_TYPE_REF_GRAD   1     // Reference gradiometer
#define CTF_SEN_TYPE_EEG        9     // EEG channel
#define CTF_SEN_TYPE_STIM      11     // Stimulus/trigger
#define CTF_SEN_TYPE_ADC       18     // ADC channel
#define CTF_SEN_TYPE_DAC       17     // DAC channel

// CTF res4 structure sizes
#define CTF_RES4_HEADER_SIZE    1844
#define CTF_SENSOR_RECORD_SIZE  1328
#define CTF_SENSOR_NAME_SIZE    32
#define CTF_MAX_COILS           8
#define CTF_COIL_POS_SIZE       56    // 7 doubles per coil

//=============================================================================================================
// CTF data structures
//=============================================================================================================

struct CtfCoilPos {
    double x, y, z;            // position in m
    double ox, oy, oz;         // orientation
    double area;               // coil area in m^2
};

struct CtfSensorInfo {
    char name[CTF_SENSOR_NAME_SIZE];
    int sensorType;
    int nCoils;
    CtfCoilPos coils[CTF_MAX_COILS];
    double properGain;         // sensor gain
    double qGain;              // electronics gain
    double ioGain;             // I/O unit gain
    double ioOffset;
};

struct CtfDatasetInfo {
    QString dsPath;
    QString res4Path;
    QString meg4Path;

    int nChannels;
    int nSamples;              // total samples per channel per trial
    int nTrials;
    double sfreq;
    int dataFormat;            // 1=short, 4=int, 8=double

    QList<CtfSensorInfo> sensors;
};

//=============================================================================================================
// Read big-endian integers/doubles from QDataStream
//=============================================================================================================

static qint32 readInt32BE(QDataStream &ds)
{
    qint32 val;
    ds >> val;
    return val;
}

static double readFloat64BE(QDataStream &ds)
{
    double val;
    ds >> val;
    return val;
}

//=============================================================================================================
// Parse CTF res4 header
//=============================================================================================================

static bool parseRes4(CtfDatasetInfo &info)
{
    QFile file(info.res4Path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open res4: %s", qPrintable(info.res4Path));
        return false;
    }

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    // Skip header identifier (8 bytes: "MEG41RS" or "MEG42RS")
    char headerTag[9] = {0};
    ds.readRawData(headerTag, 8);
    printf("Res4 tag: %s\n", headerTag);

    // Skip appName (256 bytes), DataOrigin (256 bytes), DataDescription (256 bytes)
    ds.skipRawData(256 + 256 + 256);

    // General header fields
    qint16 nSensors;
    ds >> nSensors;
    info.nChannels = nSensors;

    // Skip some reserved bytes (2 bytes)
    ds.skipRawData(2);

    // Read sample rate and sample count
    // After the padding, at offset 1844 - some_offset
    // The res4 layout varies by version, but typically:
    // Offset 780: nSamples (int32)
    // Offset 784: nTrials (int16)
    // Offset 796: sampleRate (double)
    file.seek(776);
    ds.setDevice(&file);
    info.nSamples = readInt32BE(ds);
    qint16 nTrials;
    ds >> nTrials;
    info.nTrials = nTrials;

    ds.skipRawData(6); // padding to align to 8-byte boundary

    // Skip to sample rate position
    file.seek(788);
    ds.setDevice(&file);
    info.sfreq = readFloat64BE(ds);

    // Read data time (epoch length in seconds)
    double epochTime = readFloat64BE(ds);
    (void)epochTime;

    // Skip to number of samples if still 0
    if (info.nSamples <= 0 || info.sfreq <= 0) {
        // Try alternative approach: compute from epoch time
        if (info.sfreq > 0 && epochTime > 0)
            info.nSamples = static_cast<int>(epochTime * info.sfreq);
    }

    printf("Res4: %d channels, %d samples/trial, %d trials, %.1f Hz\n",
           info.nChannels, info.nSamples, info.nTrials, info.sfreq);

    // Seek to sensor records (at fixed offset after general header)
    file.seek(CTF_RES4_HEADER_SIZE);
    ds.setDevice(&file);

    // Read sensor information records
    for (int c = 0; c < info.nChannels; ++c) {
        qint64 startPos = file.pos();

        CtfSensorInfo sen;
        memset(&sen, 0, sizeof(sen));

        // Sensor name (32 bytes)
        ds.readRawData(sen.name, CTF_SENSOR_NAME_SIZE);

        // Sensor type (2 bytes)
        qint16 stype;
        ds >> stype;
        sen.sensorType = stype;

        // Skip to gains (fixed offsets within sensor record)
        // original_run_no(2), ctfCoilType(4), reserved(4)
        ds.skipRawData(2 + 4 + 4);

        // properGain, qGain, ioGain, ioOffset (each 8 bytes)
        sen.properGain = readFloat64BE(ds);
        sen.qGain = readFloat64BE(ds);
        sen.ioGain = readFloat64BE(ds);
        sen.ioOffset = readFloat64BE(ds);

        // Number of coils (2 bytes)
        qint16 nCoils;
        ds >> nCoils;
        sen.nCoils = std::min(static_cast<int>(nCoils), CTF_MAX_COILS);

        ds.skipRawData(2); // padding

        // Read coil positions
        for (int j = 0; j < sen.nCoils; ++j) {
            // CTF coordinates are in cm, convert to m
            sen.coils[j].x = readFloat64BE(ds) * 0.01;
            sen.coils[j].y = readFloat64BE(ds) * 0.01;
            sen.coils[j].z = readFloat64BE(ds) * 0.01;
            sen.coils[j].ox = readFloat64BE(ds);
            sen.coils[j].oy = readFloat64BE(ds);
            sen.coils[j].oz = readFloat64BE(ds);
            sen.coils[j].area = readFloat64BE(ds) * 1e-4; // cm^2 to m^2
        }

        info.sensors.append(sen);

        // Skip to next sensor record
        file.seek(startPos + CTF_SENSOR_RECORD_SIZE);
        ds.setDevice(&file);
    }

    file.close();
    return true;
}

//=============================================================================================================
// Read CTF meg4 data
//=============================================================================================================

static bool readMeg4Data(const CtfDatasetInfo &info, MatrixXd &data)
{
    QFile file(info.meg4Path);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open meg4: %s", qPrintable(info.meg4Path));
        return false;
    }

    // Skip meg4 header (8 bytes: "MEG41CP" or similar)
    file.seek(8);

    int nChan = info.nChannels;
    int totalSamples = info.nSamples * info.nTrials;
    data.resize(nChan, totalSamples);

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    // Read trial by trial
    for (int trial = 0; trial < info.nTrials; ++trial) {
        int colOffset = trial * info.nSamples;
        // Data is stored channel-major within each trial
        for (int c = 0; c < nChan; ++c) {
            double gainFactor = 1.0;
            const CtfSensorInfo &sen = info.sensors[c];

            // Total gain: properGain * qGain * ioGain
            if (sen.properGain != 0.0 && sen.qGain != 0.0 && sen.ioGain != 0.0)
                gainFactor = 1.0 / (sen.properGain * sen.qGain * sen.ioGain);

            for (int s = 0; s < info.nSamples; ++s) {
                double val = 0.0;
                // CTF datasets typically use 32-bit integers
                qint32 ival;
                ds >> ival;
                val = static_cast<double>(ival) * gainFactor;
                data(c, colOffset + s) = val;
            }
        }
    }

    file.close();
    printf("Read %d channels x %d samples\n", nChan, totalSamples);
    return true;
}

//=============================================================================================================
// Map CTF sensor type to FIFF channel kind
//=============================================================================================================

static void mapCtfToFiff(const CtfSensorInfo &sen, FiffChInfo &ch)
{
    ch.ch_name = QString::fromLatin1(sen.name, static_cast<int>(strnlen(sen.name, CTF_SENSOR_NAME_SIZE)));

    switch (sen.sensorType) {
    case CTF_SEN_TYPE_MEG:
        ch.kind = FIFFV_MEG_CH;
        // CTF axial gradiometers have 2 coils, magnetometers have 1
        if (sen.nCoils >= 2)
            ch.chpos.coil_type = FIFFV_COIL_CTF_GRAD;
        else
            ch.chpos.coil_type = FIFFV_COIL_CTF_REF_MAG;
        ch.unit = FIFF_UNIT_T;
        break;

    case CTF_SEN_TYPE_REF_MAG:
        ch.kind = FIFFV_REF_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_CTF_REF_MAG;
        ch.unit = FIFF_UNIT_T;
        break;

    case CTF_SEN_TYPE_REF_GRAD:
        ch.kind = FIFFV_REF_MEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_CTF_REF_GRAD;
        ch.unit = FIFF_UNIT_T;
        break;

    case CTF_SEN_TYPE_EEG:
        ch.kind = FIFFV_EEG_CH;
        ch.chpos.coil_type = FIFFV_COIL_EEG;
        ch.unit = FIFF_UNIT_V;
        break;

    case CTF_SEN_TYPE_STIM:
        ch.kind = FIFFV_STIM_CH;
        ch.chpos.coil_type = FIFFV_COIL_NONE;
        ch.unit = FIFF_UNIT_V;
        break;

    default:
        ch.kind = FIFFV_MISC_CH;
        ch.chpos.coil_type = FIFFV_COIL_NONE;
        ch.unit = FIFF_UNIT_V;
        break;
    }

    // Set coil location from first coil
    if (sen.nCoils > 0) {
        // FIFF loc: r0(3), ex(3), ey(3), ez(3) = 12 values
        ch.chpos.r0(0) = sen.coils[0].x;
        ch.chpos.r0(1) = sen.coils[0].y;
        ch.chpos.r0(2) = sen.coils[0].z;
        // Orientation as ez
        ch.chpos.ez(0) = sen.coils[0].ox;
        ch.chpos.ez(1) = sen.coils[0].oy;
        ch.chpos.ez(2) = sen.coils[0].oz;
    }

    ch.cal = 1.0;
    ch.range = 1.0;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_ctf2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert CTF .ds MEG datasets to FIFF format.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dsOpt("ds", "Input CTF .ds directory.", "dir");
    parser.addOption(dsOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString dsPath = parser.value(dsOpt);
    QString outFile = parser.value(outOpt);

    if (dsPath.isEmpty()) { qCritical("--ds is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Locate res4 and meg4 files in .ds directory
    QDir dsDir(dsPath);
    if (!dsDir.exists()) {
        qCritical("Dataset directory does not exist: %s", qPrintable(dsPath));
        return 1;
    }

    CtfDatasetInfo info;
    info.dsPath = dsPath;

    // Find res4 file
    QStringList res4Files = dsDir.entryList(QStringList() << "*.res4", QDir::Files);
    if (res4Files.isEmpty()) {
        qCritical("No .res4 file found in %s", qPrintable(dsPath));
        return 1;
    }
    info.res4Path = dsDir.filePath(res4Files.first());

    // Find meg4 file
    QStringList meg4Files = dsDir.entryList(QStringList() << "*.meg4", QDir::Files);
    if (meg4Files.isEmpty()) {
        qCritical("No .meg4 file found in %s", qPrintable(dsPath));
        return 1;
    }
    info.meg4Path = dsDir.filePath(meg4Files.first());

    printf("CTF dataset: %s\n", qPrintable(dsPath));
    printf("  res4: %s\n", qPrintable(info.res4Path));
    printf("  meg4: %s\n", qPrintable(info.meg4Path));

    // Parse res4 header
    if (!parseRes4(info))
        return 1;

    // Read meg4 data
    MatrixXd data;
    if (!readMeg4Data(info, data))
        return 1;

    // Build FiffInfo
    int nChan = info.nChannels;
    FiffInfo fiffInfo;
    fiffInfo.sfreq = info.sfreq;
    fiffInfo.nchan = nChan;
    fiffInfo.meas_date[0] = static_cast<fiff_int_t>(QDateTime::currentDateTime().toSecsSinceEpoch()); fiffInfo.meas_date[1] = 0;

    // Set coordinate transform (CTF device -> head, identity for now)
    FiffCoordTrans devHeadTrans;
    devHeadTrans.from = FIFFV_COORD_DEVICE;
    devHeadTrans.to = FIFFV_COORD_HEAD;
    devHeadTrans.trans = Matrix4f::Identity();
    devHeadTrans.invtrans = Matrix4f::Identity();
    fiffInfo.dev_head_t = devHeadTrans;

    for (int c = 0; c < nChan; ++c) {
        FiffChInfo ch;
        ch.scanNo = c + 1;
        ch.logNo = c + 1;
        mapCtfToFiff(info.sensors[c], ch);
        fiffInfo.chs.append(ch);
        fiffInfo.ch_names.append(ch.ch_name);
    }

    // Write FIFF output
    QFile outF(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(outF, fiffInfo, cals);
    if (!stream) {
        qCritical("Cannot open output: %s", qPrintable(outFile));
        return 1;
    }

    int nSamples = data.cols();
    int chunkSize = 10000;
    for (int start = 0; start < nSamples; start += chunkSize) {
        int end = std::min(start + chunkSize, nSamples);
        MatrixXd chunk = data.block(0, start, nChan, end - start);
        stream->write_raw_buffer(chunk);
    }

    stream->finish_writing_raw();
    printf("Written FIFF: %s (%d channels, %d samples, %.1f Hz)\n",
           qPrintable(outFile), nChan, nSamples, info.sfreq);

    return 0;
}
