//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     March, 2026
 * @brief    Implements mne_create_comp_data: create CTF compensation data from ASCII matrices.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/fiff/fiff_ctf_comp.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

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
        Log::critical("Cannot open matrix file: {}", filename.toStdString());
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
        Log::critical("Invalid dimensions in matrix file: {} x {}", nrow, ncol);
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
            Log::critical("Unexpected end of matrix file at row {}", r);
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

    Log::info("Read matrix: {} x {}", nrow, ncol);
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
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
        Log::critical("Input matrix file (--in) is required.");
        return 1;
    }
    if (!parser.isSet(kindOpt)) {
        Log::critical("Compensation kind (--kind) is required.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("Output file (--out) is required.");
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
    auto stream = FiffStream::start_file(outFile.toStdU16String());
    if (!stream) {
        Log::critical("Cannot create output FIFF file.");
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
    StringList rows;
    StringList cols;
    for (const QString& name : rowNames)
        rows.push_back(name.toStdString());
    for (const QString& name : colNames)
        cols.push_back(name.toStdString());
    FiffNamedMatrix namedMat(nrow, ncol, rows, cols, data);
    stream->write_named_matrix(FIFF_MNE_CTF_COMP_DATA, namedMat);

    stream->end_block(FIFFB_MNE_CTF_COMP_DATA);
    stream->end_block(FIFFB_MNE_CTF_COMP);
    stream->end_file();

    Log::info("Wrote CTF compensation data (kind={}, {}x{}, {}) to {}", ctfKind, nrow, ncol, calibrated ? "calibrated" : "uncalibrated", outFile.toStdString());

    return 0;
}
