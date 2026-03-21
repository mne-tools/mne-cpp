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
 * @brief    Apply CTF software compensation to raw data.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_raw_data.h>
#include <fiff/fiff_info.h>
#include <fiff/fiff_ctf_comp.h>
#include <fiff/fiff_stream.h>
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

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_compensate_data");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Apply CTF software compensation to raw FIFF data.\n\nSets the desired compensation grade (0-3).");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input raw FIFF file.", "file");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);

    QCommandLineOption gradeOpt("grade", "Desired compensation grade (0, 1, 2, or 3).", "value");
    parser.addOption(gradeOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString outFile = parser.value(outOpt);
    int grade = parser.isSet(gradeOpt) ? parser.value(gradeOpt).toInt() : -1;

    if (inFile.isEmpty()) { qCritical("--in is required."); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); return 1; }
    if (grade < 0) { qCritical("--grade is required (0, 1, 2, or 3)."); return 1; }

    // Open input file
    QFile fileIn(inFile);
    FiffRawData raw(fileIn);
    if (raw.info.isEmpty()) {
        qCritical("Cannot read raw data from: %s", qPrintable(inFile));
        return 1;
    }

    qint32 currentComp = raw.info.get_current_comp();
    printf("Current compensation grade: %d\n", currentComp);
    printf("Desired compensation grade: %d\n", grade);

    if (currentComp == grade) {
        printf("Data already at desired compensation grade. No changes needed.\n");
        // Just copy the file
        QFile::copy(inFile, outFile);
        return 0;
    }

    // Make compensator matrix
    FiffCtfComp comp;
    if (!raw.info.make_compensator(currentComp, grade, comp)) {
        qCritical("Cannot create compensator from grade %d to %d", currentComp, grade);
        return 1;
    }
    printf("Compensation matrix created (%d -> %d)\n", currentComp, grade);

    // Read all data
    MatrixXd data;
    MatrixXd times;

    if (!raw.read_raw_segment(data, times)) {
        qCritical("Cannot read raw data segment");
        return 1;
    }
    printf("Read %d channels x %d samples\n", (int)data.rows(), (int)data.cols());

    // Apply compensation
    data = comp.data->data.cast<double>() * data;
    printf("Compensation applied.\n");

    // Update compensation info
    raw.info.set_current_comp(grade);

    // Write output
    QFile fileOut(outFile);
    Eigen::RowVectorXd cals;
    FiffStream::SPtr outStream = FiffStream::start_writing_raw(fileOut, raw.info, cals);
    if (!outStream) {
        qCritical("Cannot open output file: %s", qPrintable(outFile));
        return 1;
    }

    // Write data in chunks
    int chunkSize = 10000;
    for (int start = 0; start < data.cols(); start += chunkSize) {
        int end = std::min(start + chunkSize, (int)data.cols());
        MatrixXd chunk = data.block(0, start, data.rows(), end - start);
        outStream->write_raw_buffer(chunk);
    }

    outStream->finish_writing_raw();
    printf("Written compensated data to: %s\n", qPrintable(outFile));

    return 0;
}
