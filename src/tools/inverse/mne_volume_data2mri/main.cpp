//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Implements mne_volume_data2mri: convert source estimates to MRI overlay volumes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/fiff/fiff.h>
#include <mne/inv/inv_source_estimate.h>
#include <mne/mne/mne.h>
#include <mne/mne/mne_source_space.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QTextStream>
#include <QDataStream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

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

static bool readWFile(const QString& filename, VectorXd& data, VectorXi& vertices)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        Log::critical("Cannot open w file: {}", filename.toStdString());
        return false;
    }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::BigEndian);

    // Skip 2-byte header
    quint16 header;
    in >> header;

    // Number of vertices (3 bytes)
    quint8 b1, b2, b3;
    in >> b1 >> b2 >> b3;
    qint32 nvert = (b1 << 16) | (b2 << 8) | b3;

    vertices.resize(nvert);
    data.resize(nvert);

    for (int i = 0; i < nvert; i++) {
        // Vertex number (3 bytes)
        in >> b1 >> b2 >> b3;
        vertices(i) = (b1 << 16) | (b2 << 8) | b3;

        // Value (float)
        float val;
        in >> val;
        data(i) = val;
    }
    return true;
}

//=============================================================================================================

static bool readStcFile(const QString& filename, MatrixXd& data, VectorXi& vertices,
                        float& tmin, float& tstep)
{
    auto read = InvSourceEstimate::read(filename.toStdU16String());
    if (!read) {
        Log::critical("{}", read.error().what());
        return false;
    }
    const InvSourceEstimate& stc = *read;

    data = stc.data;
    vertices = stc.vertices;
    tmin = stc.tmin;
    tstep = stc.tstep;
    return true;
}

//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_volume_data2mri");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Map source estimate data to MRI vertex space.\n\nReads volume source space and STC/W estimates.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption srcOpt("src", "Source space FIFF file.", "file");
    parser.addOption(srcOpt);
    QCommandLineOption stcOpt("stc", "STC source estimate file.", "file");
    parser.addOption(stcOpt);
    QCommandLineOption wOpt("w", "W source estimate file.", "file");
    parser.addOption(wOpt);
    QCommandLineOption outOpt("out", "Output MRI file (mgh format).", "file");
    parser.addOption(outOpt);
    QCommandLineOption tpointOpt("tpoint", "Time point index for STC files.", "index", "0");
    parser.addOption(tpointOpt);

    parser.process(app);

    QString srcFile = parser.value(srcOpt);
    QString stcFile = parser.value(stcOpt);
    QString wFile = parser.value(wOpt);
    QString outFile = parser.value(outOpt);
    int tpoint = parser.value(tpointOpt).toInt();

    if (srcFile.isEmpty()) {
        Log::critical("Source space file (--src) is required.");
        return 1;
    }
    if (stcFile.isEmpty() && wFile.isEmpty()) {
        Log::info("Either --stc or --w must be specified.");
        return 1;
    }
    if (outFile.isEmpty()) {
        Log::critical("Output file (--out) is required.");
        return 1;
    }

    //
    // Read source space
    //
    std::vector<std::unique_ptr<MNESourceSpace>> spaces;
    if (MNESourceSpace::read_source_spaces(srcFile.toStdU16String(), spaces) != FIFF_OK || spaces.empty()) {
        Log::critical("Failed to read source space from: {}", srcFile.toStdString());
        return 1;
    }

    Log::info("Read {} source spaces", static_cast<long long>(spaces.size()));

    //
    // Read source estimate data
    //
    VectorXd values;
    VectorXi vertices;

    if (!stcFile.isEmpty()) {
        MatrixXd stcData;
        float tmin;
        float tstep;
        if (!readStcFile(stcFile, stcData, vertices, tmin, tstep)) {
            Log::critical("Failed to read stc file.");
            return 1;
        }
        if (tpoint < 0 || tpoint >= static_cast<int>(stcData.cols())) {
            Log::info("Time point {} out of range (0-{})", tpoint, static_cast<long>(stcData.cols() - 1));
            return 1;
        }
        values = stcData.col(tpoint);
        Log::info("Read STC: {} vertices, {} time points, using tpoint {}", static_cast<long>(stcData.rows()), static_cast<long>(stcData.cols()), tpoint);
    } else {
        if (!readWFile(wFile, values, vertices)) {
            Log::critical("Failed to read w file.");
            return 1;
        }
        Log::info("Read W: {} vertices", static_cast<long>(values.size()));
    }

    //
    // For volume source spaces, map source data to vertex space
    //
    bool found = false;
    int offset = 0;
    for (size_t s = 0; s < spaces.size(); s++) {
        const auto& sp = *spaces[s];

        // Build vertex value map for this source space
        VectorXd vertValues = VectorXd::Zero(sp.np);

        for (int i = 0; i < vertices.size(); i++) {
            int vi = vertices(i) - offset;
            if (vi >= 0 && vi < sp.np)
                vertValues(vi) = values(i);
        }
        offset += sp.nuse;

        // Check for non-zero values
        int nonZero = static_cast<int>((vertValues.array() != 0.0).count());
        if (nonZero == 0)
            continue;

        found = true;

        // Write vertex values as text (vertex index and value)
        QFile outF(outFile);
        if (!outF.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Log::critical("Cannot write output file: {}", outFile.toStdString());
            return 1;
        }
        QTextStream out(&outF);
        out << "# MNE volume data overlay\n";
        out << "# Source space: " << srcFile << "\n";
        out << "# Vertices: " << sp.np << "\n";
        out << "# Non-zero: " << nonZero << "\n";
        for (int i = 0; i < sp.np; i++) {
            if (vertValues(i) != 0.0) {
                out << i << " "
                    << sp.rr(i, 0) << " " << sp.rr(i, 1) << " " << sp.rr(i, 2) << " "
                    << vertValues(i) << "\n";
            }
        }
        Log::info("Wrote {} non-zero vertex values to {}", nonZero, outFile.toStdString());
        break;
    }

    if (!found) {
        Log::info("No matching source data found in any source space.");
        return 1;
    }

    return 0;
}
