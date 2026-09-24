//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Fix STI 014 channel by combining STI 001-006.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ch_info.h>
#include <fiff/fiff_file.h>
#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDebug>

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

static int findChannelByName(const FiffInfo &info, const QString &name)
{
    for (int i = 0; i < info.chs.size(); ++i) {
        if (info.chs[i].ch_name.trimmed() == name)
            return i;
    }
    return -1;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_fix_stim14");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Fix STI 014 channel by combining STI 001-006.\n\n"
                                     "STI014 = STI001 + 2*STI002 + 4*STI003 + 8*STI004 + 16*STI005 + 32*STI006");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption rawOpt("raw", "Input raw FIFF file.", "file");
    parser.addOption(rawOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    parser.process(app);

    QString rawFile = parser.value(rawOpt);
    QString outFile = parser.value(outOpt);

    if (rawFile.isEmpty()) { qCritical("--raw is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }

    // Open raw data
    QFile fileIn(rawFile);
    FiffRawData raw(fileIn);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw data from: %s", qPrintable(rawFile));
        return 1;
    }

    // Find STI channels
    const QStringList stiNames = {"STI 001", "STI 002", "STI 003", "STI 004", "STI 005", "STI 006"};
    const double weights[] = {1.0, 2.0, 4.0, 8.0, 16.0, 32.0};

    int stiIndices[6];
    for (int i = 0; i < 6; ++i) {
        stiIndices[i] = findChannelByName(raw.info, stiNames[i]);
        if (stiIndices[i] < 0) {
            qCritical("Cannot find channel: %s", qPrintable(stiNames[i]));
            return 1;
        }
        qInfo("Found %s at index %d" , qPrintable(stiNames[i]), stiIndices[i]);
    }

    int sti14Idx = findChannelByName(raw.info, "STI 014");
    if (sti14Idx < 0) {
        // Try alternate name
        sti14Idx = findChannelByName(raw.info, "STI014");
    }
    if (sti14Idx < 0) {
        qCritical("Cannot find STI 014 channel.");
        return 1;
    }
    qInfo("Found STI 014 at index %d" , sti14Idx);

    // Read all data
    MatrixXd data;
    MatrixXd times;
    if (!raw.read_raw_segment(data, times)) {
        qCritical("Cannot read raw data segment.");
        return 1;
    }

    // Combine: STI014 = STI001 + 2*STI002 + 4*STI003 + 8*STI004 + 16*STI005 + 32*STI006
    data.row(sti14Idx).setZero();
    for (int i = 0; i < 6; ++i) {
        for (int s = 0; s < data.cols(); ++s) {
            if (data(stiIndices[i], s) > 0.5) {
                data(sti14Idx, s) += weights[i];
            }
        }
    }
    qInfo("STI 014 recomputed from STI 001-006.");

    // Write output
    QFile fileOut(outFile);
    RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(fileOut, raw.info, cals);
    if (!outStream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    fiff_int_t firstSample = raw.first_samp;
    outStream->write_int(FIFF_FIRST_SAMPLE, &firstSample);

    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk, cals);
    }
    outStream->finish_writing_raw();

    qInfo("Written fixed raw data to: %s" , qPrintable(outFile));
    return 0;
}
