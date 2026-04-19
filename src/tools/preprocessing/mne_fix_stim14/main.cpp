//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Fix STI 014 channel by combining STI 001-006.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_raw_data.h>
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
        printf("Found %s at index %d\n", qPrintable(stiNames[i]), stiIndices[i]);
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
    printf("Found STI 014 at index %d\n", sti14Idx);

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
    printf("STI 014 recomputed from STI 001-006.\n");

    // Write output
    QFile fileOut(outFile);
    RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(fileOut, raw.info, cals);
    if (!outStream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, static_cast<int>(data.cols()));
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk);
    }
    outStream->finish_writing_raw();

    printf("Written fixed raw data to: %s\n", qPrintable(outFile));
    return 0;
}
