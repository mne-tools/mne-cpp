//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Convert CTF .ds MEG datasets to FIFF format.
 *
 *           Reads the CTF .ds directory structure (res4 header + meg4 data)
 *           and writes a standard FIFF raw file.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_stream.h>
#include <mne/fiff/fiff_info.h>
#include <mne/fiff/fiff_ch_info.h>
#include <mne/fiff/fiff_types.h>
#include <mne/base/log.h>
#include <mne/base/string_utils.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

// CTF sensor types (from res4 file)
constexpr auto CTF_SEN_TYPE_MEG = 5;      // MEG magnetometer/gradiometer
constexpr auto CTF_SEN_TYPE_REF_MAG = 0;  // Reference magnetometer
constexpr auto CTF_SEN_TYPE_REF_GRAD = 1; // Reference gradiometer
constexpr auto CTF_SEN_TYPE_EEG = 9;      // EEG channel
constexpr auto CTF_SEN_TYPE_STIM = 11;    // Stimulus/trigger

// CTF res4 structure sizes
constexpr auto CTF_RES4_HEADER_SIZE = 1844;
constexpr auto CTF_SENSOR_RECORD_SIZE = 1328;
constexpr auto CTF_SENSOR_NAME_SIZE = 32;
constexpr auto CTF_MAX_COILS = 8;

//=============================================================================================================
// CTF data structures
//=============================================================================================================

struct CtfCoilPos
{
    double x, y, z;    // position in m
    double ox, oy, oz; // orientation
    double area;       // coil area in m^2
};

struct CtfSensorInfo
{
    char name[CTF_SENSOR_NAME_SIZE];
    int sensorType;
    int nCoils;
    CtfCoilPos coils[CTF_MAX_COILS];
    double properGain; // sensor gain
    double qGain;      // electronics gain
    double ioGain;     // I/O unit gain
    double ioOffset;
};

struct CtfDatasetInfo
{
    QString dsPath;
    QString res4Path;
    QString meg4Path;

    int nChannels;
    int nSamples; // total samples per channel per trial
    int nTrials;
    double sfreq;
    int dataFormat; // 1=short, 4=int, 8=double

    QList<CtfSensorInfo> sensors;
};

//=============================================================================================================
// Read big-endian integers/doubles from QDataStream
//=============================================================================================================

static qint32 readInt32BE(QDataStream& ds)
{
    qint32 val;
    ds >> val;
    return val;
}

static double readFloat64BE(QDataStream& ds)
{
    double val;
    ds >> val;
    return val;
}

//=============================================================================================================
// Parse CTF res4 header
//=============================================================================================================

static bool parseRes4(CtfDatasetInfo& info)
{
    QFile file(info.res4Path);
    if (!file.open(QIODevice::ReadOnly)) {
        Log::critical("Cannot open res4: {}", info.res4Path.toStdString());
        return false;
    }

    QDataStream ds(&file);
    ds.setByteOrder(QDataStream::BigEndian);
    ds.setFloatingPointPrecision(QDataStream::DoublePrecision);

    // Skip header identifier (8 bytes: "MEG41RS" or "MEG42RS")
    char headerTag[9] = {0};
    ds.readRawData(headerTag, 8);
    Log::info("Res4 tag: {}", headerTag);

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

    // Skip to number of samples if still 0
    if (info.nSamples <= 0 || info.sfreq <= 0) {
        // Try alternative approach: compute from epoch time
        if (info.sfreq > 0 && epochTime > 0)
            info.nSamples = static_cast<int>(epochTime * info.sfreq);
    }

    Log::info("Res4: {} channels, {} samples/trial, {} trials, {:.1f} Hz", info.nChannels, info.nSamples, info.nTrials, info.sfreq);

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

static bool readMeg4Data(const CtfDatasetInfo& info, MatrixXd& data)
{
    QFile file(info.meg4Path);
    if (!file.open(QIODevice::ReadOnly)) {
        Log::critical("Cannot open meg4: {}", info.meg4Path.toStdString());
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
            const CtfSensorInfo& sen = info.sensors[c];

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
    Log::info("Read {} channels x {} samples", nChan, totalSamples);
    return true;
}

//=============================================================================================================
// Map CTF sensor type to FIFF channel kind
//=============================================================================================================

static void mapCtfToFiff(const CtfSensorInfo& sen, FiffChInfo& ch)
{
    ch.ch_name = (QString::fromLatin1(sen.name, static_cast<int>(strnlen(sen.name, CTF_SENSOR_NAME_SIZE)))).toStdString();

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

int main(int argc, char* argv[])
{
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

    if (dsPath.isEmpty()) {
        Log::critical("--ds is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("--out is required.");
        return 1;
    }

    // Locate res4 and meg4 files in .ds directory
    QDir dsDir(dsPath);
    if (!dsDir.exists()) {
        Log::critical("Dataset directory does not exist: {}", dsPath.toStdString());
        return 1;
    }

    CtfDatasetInfo info;
    info.dsPath = dsPath;

    // Find res4 file
    QStringList res4Files = dsDir.entryList(QStringList() << "*.res4", QDir::Files);
    if (res4Files.isEmpty()) {
        Log::critical("No .res4 file found in {}", dsPath.toStdString());
        return 1;
    }
    info.res4Path = dsDir.filePath(res4Files.first());

    // Find meg4 file
    QStringList meg4Files = dsDir.entryList(QStringList() << "*.meg4", QDir::Files);
    if (meg4Files.isEmpty()) {
        Log::critical("No .meg4 file found in {}", dsPath.toStdString());
        return 1;
    }
    info.meg4Path = dsDir.filePath(meg4Files.first());

    Log::info("CTF dataset: {}", dsPath.toStdString());
    Log::info("  res4: {}", info.res4Path.toStdString());
    Log::info("  meg4: {}", info.meg4Path.toStdString());

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
    fiffInfo.meas_date[0] = static_cast<fiff_int_t>(QDateTime::currentDateTime().toSecsSinceEpoch());
    fiffInfo.meas_date[1] = 0;

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
        fiffInfo.chs.push_back(ch);
        fiffInfo.ch_names.push_back(ch.ch_name);
    }

    // Write FIFF output
    Eigen::RowVectorXd cals;
    auto stream = FiffStream::start_writing_raw(outFile.toStdU16String(), fiffInfo, cals);
    if (!stream) {
        Log::critical("Cannot open output: {}", outFile.toStdString());
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
    Log::info("Written FIFF: {} ({} channels, {} samples, {:.1f} Hz)", outFile.toStdString(), nChan, nSamples, info.sfreq);

    return 0;
}
