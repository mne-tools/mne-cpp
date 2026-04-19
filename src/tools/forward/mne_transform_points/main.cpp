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
 * @brief    Transform 3D points between coordinate frames using a FIFF coordinate transform.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_constants.h>

#include <utils/generics/mne_logger.h>

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
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static int frameFromName(const QString& name)
{
    QString lower = name.toLower();
    if (lower == "head")        return FIFFV_COORD_HEAD;
    if (lower == "mri")         return FIFFV_COORD_MRI;
    if (lower == "device")      return FIFFV_COORD_DEVICE;
    if (lower == "isotrak")     return FIFFV_COORD_ISOTRAK;
    if (lower == "hpi")         return FIFFV_COORD_HPI;
    return -1;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_transform_points");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Transform 3D points between coordinate frames.\n\nReads a FIFF coordinate transform and applies it to a set of 3D points.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption transOpt("trans", "FIFF coordinate transform file.", "name");
    parser.addOption(transOpt);

    QCommandLineOption inOpt("in", "Input text file (one point per line: x y z).", "name");
    parser.addOption(inOpt);

    QCommandLineOption outOpt("out", "Output text file.", "name");
    parser.addOption(outOpt);

    QCommandLineOption fromOpt("from", "Source frame name (head, mri, device).", "name");
    parser.addOption(fromOpt);

    QCommandLineOption toOpt("to", "Target frame name (head, mri, device).", "name");
    parser.addOption(toOpt);

    parser.process(app);

    QString transName = parser.value(transOpt);
    QString inName = parser.value(inOpt);
    QString outName = parser.value(outOpt);
    QString fromName = parser.value(fromOpt);
    QString toName = parser.value(toOpt);

    if (transName.isEmpty() || inName.isEmpty() || outName.isEmpty() || fromName.isEmpty() || toName.isEmpty()) {
        qCritical("All options --trans, --in, --out, --from, and --to are required.");
        parser.showHelp(1);
    }

    int fromFrame = frameFromName(fromName);
    int toFrame = frameFromName(toName);

    if (fromFrame < 0) {
        qCritical("Unknown source frame: %s", qPrintable(fromName));
        return 1;
    }
    if (toFrame < 0) {
        qCritical("Unknown target frame: %s", qPrintable(toName));
        return 1;
    }

    // Read coordinate transform
    QFile transFile(transName);
    FiffCoordTrans trans;
    if (!FiffCoordTrans::read(transFile, trans)) {
        qCritical("Cannot read coordinate transform from: %s", qPrintable(transName));
        return 1;
    }

    fprintf(stderr, "Read transform: %s -> %s\n",
            qPrintable(FiffCoordTrans::frame_name(trans.from)),
            qPrintable(FiffCoordTrans::frame_name(trans.to)));

    // Determine if we need forward or inverse
    bool useInverse = false;
    if (trans.from == fromFrame && trans.to == toFrame) {
        useInverse = false;
    } else if (trans.from == toFrame && trans.to == fromFrame) {
        useInverse = true;
    } else {
        qCritical("Transform (%s -> %s) does not match requested frames (%s -> %s).",
                  qPrintable(FiffCoordTrans::frame_name(trans.from)),
                  qPrintable(FiffCoordTrans::frame_name(trans.to)),
                  qPrintable(fromName), qPrintable(toName));
        return 1;
    }

    // Read input points
    QFile inFile(inName);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical("Cannot open input file: %s", qPrintable(inName));
        return 1;
    }

    QList<Vector3f> points;
    QTextStream inStream(&inFile);
    while (!inStream.atEnd()) {
        QString line = inStream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() < 3) {
            qCritical("Invalid line in input file: %s", qPrintable(line));
            return 1;
        }
        bool ok1, ok2, ok3;
        float x = parts[0].toFloat(&ok1);
        float y = parts[1].toFloat(&ok2);
        float z = parts[2].toFloat(&ok3);
        if (!ok1 || !ok2 || !ok3) {
            qCritical("Cannot parse coordinates: %s", qPrintable(line));
            return 1;
        }
        points.append(Vector3f(x, y, z));
    }
    inFile.close();

    fprintf(stderr, "Read %lld points from %s\n", static_cast<long long>(points.size()), qPrintable(inName));

    // Build matrix from points
    MatrixX3f rr(points.size(), 3);
    for (int i = 0; i < points.size(); ++i)
        rr.row(i) = points[i].transpose();

    // Apply transform
    MatrixX3f transformed;
    if (useInverse) {
        transformed = trans.apply_inverse_trans(rr);
        fprintf(stderr, "Applied inverse transform.\n");
    } else {
        transformed = trans.apply_trans(rr);
        fprintf(stderr, "Applied forward transform.\n");
    }

    // Write output
    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot open output file: %s", qPrintable(outName));
        return 1;
    }

    QTextStream outStream(&outFile);
    outStream.setRealNumberPrecision(8);
    outStream.setRealNumberNotation(QTextStream::FixedNotation);
    for (int i = 0; i < transformed.rows(); ++i) {
        outStream << transformed(i, 0) << " "
                  << transformed(i, 1) << " "
                  << transformed(i, 2) << "\n";
    }
    outFile.close();

    fprintf(stderr, "Wrote %d transformed points to %s\n", static_cast<int>(transformed.rows()), qPrintable(outName));

    return 0;
}
