//=============================================================================================================
/**
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
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
 * @brief    Implements mne_create_comp_data: create CTF compensation data from ASCII matrices.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <fiff/fiff_ctf_comp.h>
#include <utils/generics/applicationlogger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
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
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static bool readAsciiMatrix(const QString& filename,
                            MatrixXd& data,
                            QStringList& rowNames,
                            QStringList& colNames)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open matrix file:" << filename;
        return false;
    }
    QTextStream in(&file);

    // First line: nrow ncol
    int nrow = 0, ncol = 0;
    QString dimLine = in.readLine().trimmed();
    QStringList dimParts = dimLine.split(QRegularExpression("\\s+"));
    if (dimParts.size() >= 2) {
        nrow = dimParts[0].toInt();
        ncol = dimParts[1].toInt();
    }
    if (nrow <= 0 || ncol <= 0) {
        qCritical() << "Invalid dimensions in matrix file:" << nrow << "x" << ncol;
        return false;
    }

    // Second line: column names
    QString colLine = in.readLine().trimmed();
    colNames = colLine.split(QRegularExpression("\\s+"));
    while (colNames.size() < ncol) {
        colNames.append(QString("col%1").arg(colNames.size() + 1));
    }
    if (colNames.size() > ncol)
        colNames = colNames.mid(0, ncol);

    // Data lines: row_name val1 val2 ...
    data.resize(nrow, ncol);
    rowNames.clear();
    for (int r = 0; r < nrow; r++) {
        if (in.atEnd()) {
            qCritical() << "Unexpected end of matrix file at row" << r;
            return false;
        }
        QString line = in.readLine().trimmed();
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() >= ncol + 1) {
            rowNames.append(parts[0]);
            for (int c = 0; c < ncol; c++) {
                data(r, c) = parts[c + 1].toDouble();
            }
        } else if (parts.size() >= ncol) {
            // No row name, just values
            rowNames.append(QString("row%1").arg(r + 1));
            for (int c = 0; c < ncol; c++) {
                data(r, c) = parts[c].toDouble();
            }
        }
    }

    printf("Read matrix: %d x %d\n", nrow, ncol);
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(ApplicationLogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_create_comp_data");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Create CTF compensation data from ASCII matrix files.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption inOpt("in", "Input ASCII matrix file.", "file");
    parser.addOption(inOpt);
    QCommandLineOption kindOpt("kind", "CTF compensation kind (e.g. 101, 201, 301).", "kind");
    parser.addOption(kindOpt);
    QCommandLineOption outOpt("out", "Output FIFF file.", "file");
    parser.addOption(outOpt);
    QCommandLineOption calibratedOpt("calibrated", "Matrix data is already calibrated.");
    parser.addOption(calibratedOpt);

    parser.process(app);

    QString inFile = parser.value(inOpt);
    QString outFile = parser.value(outOpt);
    int ctfKind = parser.value(kindOpt).toInt();
    bool calibrated = parser.isSet(calibratedOpt);

    if (inFile.isEmpty()) {
        fprintf(stderr, "Input matrix file (--in) is required.\n");
        return 1;
    }
    if (!parser.isSet(kindOpt)) {
        fprintf(stderr, "Compensation kind (--kind) is required.\n");
        return 1;
    }
    if (outFile.isEmpty()) {
        fprintf(stderr, "Output file (--out) is required.\n");
        return 1;
    }

    //
    // Read the ASCII matrix
    //
    MatrixXd data;
    QStringList rowNames, colNames;
    if (!readAsciiMatrix(inFile, data, rowNames, colNames))
        return 1;

    int nrow = static_cast<int>(data.rows());
    int ncol = static_cast<int>(data.cols());

    //
    // Write FIFF output with CTF compensation data
    //
    QFile fOut(outFile);
    FiffStream::SPtr stream = FiffStream::start_file(fOut);
    if (!stream) {
        fprintf(stderr, "Cannot create output FIFF file.\n");
        return 1;
    }

    // CTF compensation block
    stream->start_block(FIFFB_MNE_CTF_COMP);
    stream->start_block(FIFFB_MNE_CTF_COMP_DATA);

    // Compensation kind
    fiff_int_t kind = ctfKind;
    stream->write_int(FIFF_MNE_CTF_COMP_KIND, &kind);

    // Calibrated flag
    fiff_int_t cal = calibrated ? 1 : 0;
    stream->write_int(FIFF_MNE_CTF_COMP_CALIBRATED, &cal);

    // Write the named matrix
    FiffNamedMatrix namedMat(nrow, ncol, rowNames, colNames, data);
    stream->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, namedMat);

    stream->end_block(FIFFB_MNE_CTF_COMP_DATA);
    stream->end_block(FIFFB_MNE_CTF_COMP);
    stream->end_file();

    printf("Wrote CTF compensation data (kind=%d, %dx%d, %s) to %s\n",
           ctfKind, nrow, ncol,
           calibrated ? "calibrated" : "uncalibrated",
           outFile.toUtf8().constData());

    return 0;
}
