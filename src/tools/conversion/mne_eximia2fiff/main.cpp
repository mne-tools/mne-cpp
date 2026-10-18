//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March, 2026
 * @brief    Implements mne_eximia2fiff: convert eXimia EEG data to FIFF format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_constants.h>
#include <fiff/fiff_dig_point.h>
#include <fiff/fiff_dig_point_set.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_stream.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>
#include <QtEndian>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace UTILSLIB;

namespace
{

// The fixed eXimia montage of mne.io.read_raw_eximia: 64 multiplexed int16 channels at 1450 Hz
constexpr int kChannels = 64;
constexpr double kSfreq = 1450.0;
constexpr double kTriggerCal = 0.0015259021896696422;
constexpr double kEogCal = 0.3814755474174106;
constexpr double kEegCal = 0.07629510948348212;

QStringList channelNames()
{
    return QStringList{"GateIn", "Trig1", "Trig2", "EOG"} + QStringLiteral("Fp1 Fpz Fp2 AF1 AFz AF2 F7 F3 F1 Fz F2 F4 F8 FT9 FT7 FC5 FC3 FC1 FCz FC2 FC4 FC6 FT8 FT10 "
                                                                           "T7 C5 C3 C1 Cz C2 C4 C6 T8 TP9 TP7 CP5 CP3 CP1 CPz CP2 CP4 CP6 TP8 TP10 "
                                                                           "P9 P7 P3 P1 Pz P2 P4 P8 P10 PO3 POz PO4 O1 Oz O2 Iz")
                                                                .split(QLatin1Char(' '));
}

FiffInfo eximiaInfo()
{
    FiffInfo info;
    info.sfreq = kSfreq;
    info.nchan = kChannels;
    const QStringList names = channelNames();
    for (int c = 0; c < kChannels; ++c) {
        FiffChInfo ch;
        ch.ch_name = names[c];
        ch.scanNo = c + 1;
        ch.logNo = c + 1;
        ch.unit = FIFF_UNIT_V;
        ch.range = 1.0f;
        if (c < 3) {
            ch.kind = FIFFV_STIM_CH;
            ch.cal = static_cast<float>(kTriggerCal);
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else if (c == 3) {
            ch.kind = FIFFV_EOG_CH;
            ch.cal = static_cast<float>(kEogCal);
            ch.chpos.coil_type = FIFFV_COIL_NONE;
        } else {
            ch.kind = FIFFV_EEG_CH;
            ch.cal = static_cast<float>(kEegCal);
            ch.chpos.coil_type = FIFFV_COIL_EEG;
        }
        info.chs.append(ch);
        info.ch_names.append(ch.ch_name);
    }
    return info;
}

// eXimia digitizer text, as MNE-C's mne_eximia2fiff reads it: "Scalp" lines naming Left, Nasion or Right are
// the fiducials and "Point ... Original" lines the EEG electrodes (number in the fifth column); x y z in mm
bool readDigitizer(const QString& path, QList<FiffDigPoint>& points)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open digitizer file %s", qPrintable(path));
        return false;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        const QStringList f = line.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
        FiffDigPoint p;
        if (line.contains(QLatin1String("Scalp"))) {
            p.kind = FIFFV_POINT_CARDINAL;
            p.ident = line.contains(QLatin1String("Left")) ? FIFFV_POINT_LPA
                : line.contains(QLatin1String("Nasion"))   ? FIFFV_POINT_NASION
                : line.contains(QLatin1String("Right"))    ? FIFFV_POINT_RPA
                                                           : 0;
        } else if (line.contains(QLatin1String("Point")) && line.contains(QLatin1String("Original")) && f.size() >= 5) {
            p.kind = FIFFV_POINT_EEG;
            p.ident = f[4].toInt();
        }
        if (p.ident == 0 || f.size() < 3)
            continue;
        bool ok[3] = {false, false, false};
        for (int k = 0; k < 3; ++k)
            p.r[k] = f[k].toFloat(&ok[k]) * 1e-3f;
        if (!ok[0] || !ok[1] || !ok[2]) {
            qCritical("Malformed digitizer line in %s: %s", qPrintable(path), qPrintable(line));
            return false;
        }
        p.coord_frame = FIFFV_COORD_UNKNOWN;
        points.append(p);
    }
    FiffDigPointSet set(points);
    if (!set.transformToHead()) {
        qCritical("%s lacks the Left, Nasion and Right fiducials needed for head coordinates", qPrintable(path));
        return false;
    }
    points = set.getList();
    return true;
}

} // namespace

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_eximia2fiff");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Convert eXimia EEG (.nxe) data to FIFF format, as mne.io.read_raw_eximia reads it.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption nxeOpt("nxe", "eXimia .nxe data file.", "file");
    parser.addOption(nxeOpt);
    QCommandLineOption digOpt("dig", "eXimia digitizer text file (fiducials and EEG electrodes).", "file");
    parser.addOption(digOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    const QString nxeFile = parser.value(nxeOpt);
    const QString outFile = parser.value(outOpt);
    if (nxeFile.isEmpty()) {
        qCritical("eXimia .nxe file (--nxe) is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        qCritical("Output file (--out) is required.");
        return 1;
    }

    QFile nxe(nxeFile);
    if (!nxe.open(QIODevice::ReadOnly)) {
        qCritical("Cannot open %s", qPrintable(nxeFile));
        return 1;
    }
    const QByteArray bytes = nxe.readAll();
    const qint64 frame = kChannels * qint64(sizeof(qint16));
    const qint64 nSamples = bytes.size() / frame;
    if (nSamples == 0) {
        qCritical("%s holds no complete sample of %d channels", qPrintable(nxeFile), kChannels);
        return 1;
    }
    if (bytes.size() % frame != 0)
        qWarning("%s ends with a partial sample; the file is likely truncated", qPrintable(nxeFile));

    FiffInfo info = eximiaInfo();
    if (parser.isSet(digOpt) && !readDigitizer(parser.value(digOpt), info.dig))
        return 1;

    Eigen::MatrixXd data(kChannels, nSamples);
    const auto* raw = reinterpret_cast<const uchar*>(bytes.constData());
    for (qint64 s = 0; s < nSamples; ++s)
        for (int c = 0; c < kChannels; ++c)
            data(c, s) = qFromLittleEndian<qint16>(raw + (s * kChannels + c) * qint64(sizeof(qint16))) * double(info.chs[c].cal);

    QFile out(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr stream = FiffStream::start_writing_raw(out, info, cals);
    if (!stream) {
        qCritical("Cannot open output: %s", qPrintable(outFile));
        return 1;
    }
    constexpr qint64 kBuffer = 10000;
    for (qint64 start = 0; start < nSamples; start += kBuffer) {
        const qint64 n = std::min(kBuffer, nSamples - start);
        if (!stream->write_raw_buffer(data.middleCols(start, n), cals)) {
            qCritical("Writing %s failed", qPrintable(outFile));
            return 1;
        }
    }
    stream->finish_writing_raw();

    qInfo("Converted %s: %d channels, %lld samples at %.0f Hz -> %s", qPrintable(QFileInfo(nxeFile).fileName()),
          kChannels, static_cast<long long>(nSamples), kSfreq, qPrintable(outFile));
    return 0;
}
