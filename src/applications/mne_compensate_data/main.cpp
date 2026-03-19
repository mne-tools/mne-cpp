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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
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
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Apply CTF software compensation to raw FIFF data.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --in <file>        Input raw FIFF file\n");
    fprintf(stderr, "  --out <file>       Output FIFF file\n");
    fprintf(stderr, "  --grade <value>    Desired compensation grade (0, 1, 2, or 3)\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString inFile;
    QString outFile;
    int grade = -1;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--in") == 0) {
            if (++k >= argc) { qCritical("--in: argument required."); return 1; }
            inFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--grade") == 0) {
            if (++k >= argc) { qCritical("--grade: argument required."); return 1; }
            grade = atoi(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (inFile.isEmpty()) { qCritical("--in is required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }
    if (grade < 0) { qCritical("--grade is required (0, 1, 2, or 3)."); usage(argv[0]); return 1; }

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
