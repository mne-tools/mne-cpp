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
 * @brief    Implements mne_volume_data2mri: convert source estimates to MRI overlay volumes.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <mne/mne_source_space.h>
#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDataStream>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/SparseCore>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Convert source estimate data to MRI overlay volume.\n\n");
    fprintf(stderr, "  --src <file>       Source space FIFF file\n");
    fprintf(stderr, "  --stc <file>       STC source estimate file\n");
    fprintf(stderr, "  --w <file>         W source estimate file\n");
    fprintf(stderr, "  --out <file>       Output MRI file (mgh format)\n");
    fprintf(stderr, "  --tpoint <int>     Time point index for STC files (default: 0)\n");
    fprintf(stderr, "  --help             Print this help\n\n");
    fprintf(stderr, "  Version: %s\n", PROGRAM_VERSION);
}

//=============================================================================================================

static bool readWFile(const QString& filename, VectorXd& data, VectorXi& vertices)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open w file:" << filename;
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
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open stc file:" << filename;
        return false;
    }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::BigEndian);

    // Time parameters
    float ftmin, ftstep;
    in >> ftmin >> ftstep;
    tmin = ftmin;
    tstep = ftstep;

    // Number of vertices
    qint32 nvert;
    in >> nvert;

    vertices.resize(nvert);
    for (int i = 0; i < nvert; i++) {
        qint32 v;
        in >> v;
        vertices(i) = v;
    }

    // Number of time points
    qint32 ntime;
    in >> ntime;

    data.resize(nvert, ntime);
    for (int t = 0; t < ntime; t++) {
        for (int i = 0; i < nvert; i++) {
            float val;
            in >> val;
            data(i, t) = val;
        }
    }
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString srcFile;
    QString stcFile;
    QString wFile;
    QString outFile;
    int tpoint = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--src") == 0 && i + 1 < argc) {
            srcFile = QString(argv[++i]);
        } else if (strcmp(argv[i], "--stc") == 0 && i + 1 < argc) {
            stcFile = QString(argv[++i]);
        } else if (strcmp(argv[i], "--w") == 0 && i + 1 < argc) {
            wFile = QString(argv[++i]);
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outFile = QString(argv[++i]);
        } else if (strcmp(argv[i], "--tpoint") == 0 && i + 1 < argc) {
            tpoint = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (srcFile.isEmpty()) {
        fprintf(stderr, "Source space file (--src) is required.\n");
        usage(argv[0]);
        return 1;
    }
    if (stcFile.isEmpty() && wFile.isEmpty()) {
        fprintf(stderr, "Either --stc or --w must be specified.\n");
        usage(argv[0]);
        return 1;
    }
    if (outFile.isEmpty()) {
        fprintf(stderr, "Output file (--out) is required.\n");
        usage(argv[0]);
        return 1;
    }

    //
    // Read source space
    //
    std::vector<std::unique_ptr<MNESourceSpace>> spaces;
    int nspace = MNESourceSpace::read_source_spaces(srcFile, spaces);
    if (nspace <= 0 || spaces.empty()) {
        fprintf(stderr, "Failed to read source space from: %s\n", srcFile.toUtf8().constData());
        return 1;
    }

    printf("Read %d source spaces\n", nspace);

    //
    // Read source estimate data
    //
    VectorXd values;
    VectorXi vertices;

    if (!stcFile.isEmpty()) {
        MatrixXd stcData;
        float tmin, tstep;
        if (!readStcFile(stcFile, stcData, vertices, tmin, tstep)) {
            fprintf(stderr, "Failed to read stc file.\n");
            return 1;
        }
        if (tpoint < 0 || tpoint >= static_cast<int>(stcData.cols())) {
            fprintf(stderr, "Time point %d out of range (0-%ld)\n",
                    tpoint, static_cast<long>(stcData.cols() - 1));
            return 1;
        }
        values = stcData.col(tpoint);
        printf("Read STC: %ld vertices, %ld time points, using tpoint %d\n",
               static_cast<long>(stcData.rows()), static_cast<long>(stcData.cols()), tpoint);
    } else {
        if (!readWFile(wFile, values, vertices)) {
            fprintf(stderr, "Failed to read w file.\n");
            return 1;
        }
        printf("Read W: %ld vertices\n", static_cast<long>(values.size()));
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
            fprintf(stderr, "Cannot write output file: %s\n", outFile.toUtf8().constData());
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
        printf("Wrote %d non-zero vertex values to %s\n",
               nonZero, outFile.toUtf8().constData());
        break;
    }

    if (!found) {
        fprintf(stderr, "No matching source data found in any source space.\n");
        return 1;
    }

    return 0;
}
