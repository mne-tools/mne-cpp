//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
 * @brief    Transform 3D points between coordinate frames using a FIFF coordinate transform.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff_coord_trans.h>
#include <mne/fiff/fiff_constants.h>

#include <mne/base/log.h>
#include <mne/base/string_utils.h>

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
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

static int frameFromName(const QString& name)
{
    QString lower = name.toLower();
    if (lower == "head")
        return FIFFV_COORD_HEAD;
    if (lower == "mri")
        return FIFFV_COORD_MRI;
    if (lower == "device")
        return FIFFV_COORD_DEVICE;
    if (lower == "isotrak")
        return FIFFV_COORD_ISOTRAK;
    if (lower == "hpi")
        return FIFFV_COORD_HPI;
    return -1;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
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
        Log::critical("All options --trans, --in, --out, --from, and --to are required.");
        parser.showHelp(1);
    }

    int fromFrame = frameFromName(fromName);
    int toFrame = frameFromName(toName);

    if (fromFrame < 0) {
        Log::critical("Unknown source frame: {}", fromName.toStdString());
        return 1;
    }
    if (toFrame < 0) {
        Log::critical("Unknown target frame: {}", toName.toStdString());
        return 1;
    }

    // Read the transform between the requested frames (inverted if stored the other way round)
    const auto read = FiffCoordTrans::read(transName.toStdU16String(), fromFrame, toFrame);
    if (!read) {
        Log::critical("{}", read.error().what());
        return 1;
    }
    const FiffCoordTrans& trans = *read;

    Log::info("Read transform: {} -> {}", std::string(FiffCoordTrans::frame_name(trans.from)), std::string(FiffCoordTrans::frame_name(trans.to)));

    // Read input points
    QFile inFile(inName);
    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Log::critical("Cannot open input file: {}", inName.toStdString());
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
            Log::critical("Invalid line in input file: {}", line.toStdString());
            return 1;
        }
        bool ok1, ok2, ok3;
        float x = parts[0].toFloat(&ok1);
        float y = parts[1].toFloat(&ok2);
        float z = parts[2].toFloat(&ok3);
        if (!ok1 || !ok2 || !ok3) {
            Log::critical("Cannot parse coordinates: {}", line.toStdString());
            return 1;
        }
        points.append(Vector3f(x, y, z));
    }
    inFile.close();

    Log::info("Read {} points from {}", static_cast<long long>(points.size()), inName.toStdString());

    // Build matrix from points
    MatrixX3f rr(points.size(), 3);
    for (int i = 0; i < points.size(); ++i)
        rr.row(i) = points[i].transpose();

    const MatrixX3f transformed = trans.apply_trans(rr);

    // Write output
    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        Log::critical("Cannot open output file: {}", outName.toStdString());
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

    Log::info("Wrote {} transformed points to {}", static_cast<int>(transformed.rows()), outName.toStdString());

    return 0;
}
