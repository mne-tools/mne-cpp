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
 * @brief    Implements mne_convert_surface: convert between surface file formats.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff.h>
#include <mne/mne.h>
#include <fs/fs_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDataStream>
#include <QDebug>
#include <QFileInfo>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FIFFLIB;
using namespace MNELIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

enum SurfaceFormat {
    FORMAT_UNKNOWN = 0,
    FORMAT_FIF,
    FORMAT_TRI,
    FORMAT_FREESURFER,
    FORMAT_SMF
};

static SurfaceFormat detectFormat(const QString& filename);
static bool readTriFile(const QString& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeTriFile(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris);
static bool readSmfFile(const QString& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeSmfFile(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris);
static bool readFifSurface(const QString& filename, MatrixX3f& rr, MatrixX3i& tris);
static bool writeFifSurface(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris, int surfId);

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Convert between surface file formats.\n\n");
    fprintf(stderr, "  --surf <file>       Input surface file\n");
    fprintf(stderr, "  --fif <file>        Input FIF surface file\n");
    fprintf(stderr, "  --tri <file>        Input ASCII tri format file\n");
    fprintf(stderr, "  --smf <file>        Input SMF format file\n");
    fprintf(stderr, "  --out <file>        Output file (format detected from extension)\n");
    fprintf(stderr, "  --outtri <file>     Output as ASCII tri format\n");
    fprintf(stderr, "  --outfif <file>     Output as FIF format\n");
    fprintf(stderr, "  --outsurf <file>    Output as FreeSurfer surface format\n");
    fprintf(stderr, "  --outsmf <file>     Output as SMF format\n");
    fprintf(stderr, "  --surfid <id>       Surface ID for FIF output (default: 4=head)\n");
    fprintf(stderr, "  --meters            Input coordinates are in meters\n");
    fprintf(stderr, "  --millimeters       Input coordinates are in millimeters\n");
    fprintf(stderr, "  --help              Print this help\n\n");
    fprintf(stderr, "  Version: %s\n", PROGRAM_VERSION);
}

//=============================================================================================================

static SurfaceFormat detectFormat(const QString& filename)
{
    if (filename.endsWith(".fif"))
        return FORMAT_FIF;
    if (filename.endsWith(".tri"))
        return FORMAT_TRI;
    if (filename.endsWith(".smf"))
        return FORMAT_SMF;
    // FreeSurfer surfaces typically have no extension or are named like lh.white, rh.pial
    QFileInfo fi(filename);
    QString base = fi.fileName();
    if (base.startsWith("lh.") || base.startsWith("rh."))
        return FORMAT_FREESURFER;
    return FORMAT_UNKNOWN;
}

//=============================================================================================================

static bool readTriFile(const QString& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open tri file:" << filename;
        return false;
    }
    QTextStream in(&file);
    int nvert = 0;
    in >> nvert;
    if (nvert <= 0) {
        qCritical() << "Invalid vertex count in tri file";
        return false;
    }
    rr.resize(nvert, 3);
    for (int i = 0; i < nvert; i++) {
        int idx;
        float x, y, z;
        in >> idx >> x >> y >> z;
        rr(i, 0) = x;
        rr(i, 1) = y;
        rr(i, 2) = z;
    }
    int ntri = 0;
    in >> ntri;
    if (ntri <= 0) {
        qCritical() << "Invalid triangle count in tri file";
        return false;
    }
    tris.resize(ntri, 3);
    for (int i = 0; i < ntri; i++) {
        int idx, v0, v1, v2;
        in >> idx >> v0 >> v1 >> v2;
        tris(i, 0) = v0 - 1; // tri format is 1-based
        tris(i, 1) = v1 - 1;
        tris(i, 2) = v2 - 1;
    }
    return true;
}

//=============================================================================================================

static bool writeTriFile(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Cannot write tri file:" << filename;
        return false;
    }
    QTextStream out(&file);
    int nvert = static_cast<int>(rr.rows());
    int ntri = static_cast<int>(tris.rows());
    out << nvert << "\n";
    for (int i = 0; i < nvert; i++) {
        out << (i + 1) << " " << rr(i, 0) << " " << rr(i, 1) << " " << rr(i, 2) << "\n";
    }
    out << ntri << "\n";
    for (int i = 0; i < ntri; i++) {
        out << (i + 1) << " " << (tris(i, 0) + 1) << " " << (tris(i, 1) + 1) << " " << (tris(i, 2) + 1) << "\n";
    }
    return true;
}

//=============================================================================================================

static bool readSmfFile(const QString& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Cannot open smf file:" << filename;
        return false;
    }
    QTextStream in(&file);
    QVector<Vector3f> verts;
    QVector<Vector3i> faces;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        QStringList parts = line.split(QRegularExpression("\\s+"));
        if (parts.size() < 4)
            continue;
        if (parts[0] == "v") {
            verts.append(Vector3f(parts[1].toFloat(), parts[2].toFloat(), parts[3].toFloat()));
        } else if (parts[0] == "f") {
            faces.append(Vector3i(parts[1].toInt() - 1, parts[2].toInt() - 1, parts[3].toInt() - 1));
        }
    }
    rr.resize(verts.size(), 3);
    for (int i = 0; i < verts.size(); i++)
        rr.row(i) = verts[i].transpose();
    tris.resize(faces.size(), 3);
    for (int i = 0; i < faces.size(); i++)
        tris.row(i) = faces[i].transpose();
    return true;
}

//=============================================================================================================

static bool writeSmfFile(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Cannot write smf file:" << filename;
        return false;
    }
    QTextStream out(&file);
    out << "# SMF surface file\n";
    for (int i = 0; i < rr.rows(); i++) {
        out << "v " << rr(i, 0) << " " << rr(i, 1) << " " << rr(i, 2) << "\n";
    }
    for (int i = 0; i < tris.rows(); i++) {
        out << "f " << (tris(i, 0) + 1) << " " << (tris(i, 1) + 1) << " " << (tris(i, 2) + 1) << "\n";
    }
    return true;
}

//=============================================================================================================

static bool readFifSurface(const QString& filename, MatrixX3f& rr, MatrixX3i& tris)
{
    MNEBem bem;
    QFile file(filename);
    FiffStream::SPtr stream(new FiffStream(&file));
    if (!stream->open()) {
        qCritical() << "Cannot open FIF file:" << filename;
        return false;
    }
    if (!MNEBem::readFromStream(stream, true, bem) || bem.size() == 0) {
        qCritical() << "No BEM surfaces found in:" << filename;
        stream->close();
        return false;
    }
    // Take the first surface
    rr = bem[0].rr;
    tris = bem[0].itris;
    stream->close();
    return true;
}

//=============================================================================================================

static bool writeFifSurface(const QString& filename, const MatrixX3f& rr, const MatrixX3i& tris, int surfId)
{
    QFile file(filename);
    FiffStream::SPtr stream = FiffStream::start_file(file);
    if (!stream) {
        qCritical() << "Cannot create FIF file:" << filename;
        return false;
    }
    stream->start_block(FIFFB_BEM);
    stream->start_block(FIFFB_BEM_SURF);

    fiff_int_t val;

    // Surface ID
    val = surfId;
    stream->write_int(FIFF_BEM_SURF_ID, &val);

    // Coordinate frame: MRI
    val = FIFFV_COORD_MRI;
    stream->write_int(FIFF_BEM_COORD_FRAME, &val);

    // Number of vertices
    val = static_cast<fiff_int_t>(rr.rows());
    stream->write_int(FIFF_BEM_SURF_NNODE, &val);

    // Number of triangles
    val = static_cast<fiff_int_t>(tris.rows());
    stream->write_int(FIFF_BEM_SURF_NTRI, &val);

    // Vertices
    stream->write_float_matrix(FIFF_BEM_SURF_NODES, rr);

    // Triangles (1-based in FIFF)
    MatrixXi tris1 = tris.array() + 1;
    stream->write_int_matrix(FIFF_BEM_SURF_TRIANGLES, tris1);

    // Normals
    MatrixX3f nn = FsSurface::compute_normals(rr, tris);
    stream->write_float_matrix(FIFF_BEM_SURF_NORMALS, nn);

    stream->end_block(FIFFB_BEM_SURF);
    stream->end_block(FIFFB_BEM);
    stream->end_file();
    return true;
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString inputFile;
    SurfaceFormat inputFormat = FORMAT_UNKNOWN;
    QString outputFile;
    SurfaceFormat outputFormat = FORMAT_UNKNOWN;
    int surfId = FIFFV_BEM_SURF_ID_HEAD;
    float scaleFactor = 1.0f; // Default: assume meters
    bool scaleSet = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--surf") == 0 && i + 1 < argc) {
            inputFile = QString(argv[++i]);
            inputFormat = FORMAT_FREESURFER;
        } else if (strcmp(argv[i], "--fif") == 0 && i + 1 < argc) {
            inputFile = QString(argv[++i]);
            inputFormat = FORMAT_FIF;
        } else if (strcmp(argv[i], "--tri") == 0 && i + 1 < argc) {
            inputFile = QString(argv[++i]);
            inputFormat = FORMAT_TRI;
        } else if (strcmp(argv[i], "--smf") == 0 && i + 1 < argc) {
            inputFile = QString(argv[++i]);
            inputFormat = FORMAT_SMF;
        } else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outputFile = QString(argv[++i]);
            outputFormat = detectFormat(outputFile);
        } else if (strcmp(argv[i], "--outtri") == 0 && i + 1 < argc) {
            outputFile = QString(argv[++i]);
            outputFormat = FORMAT_TRI;
        } else if (strcmp(argv[i], "--outfif") == 0 && i + 1 < argc) {
            outputFile = QString(argv[++i]);
            outputFormat = FORMAT_FIF;
        } else if (strcmp(argv[i], "--outsurf") == 0 && i + 1 < argc) {
            outputFile = QString(argv[++i]);
            outputFormat = FORMAT_FREESURFER;
        } else if (strcmp(argv[i], "--outsmf") == 0 && i + 1 < argc) {
            outputFile = QString(argv[++i]);
            outputFormat = FORMAT_SMF;
        } else if (strcmp(argv[i], "--surfid") == 0 && i + 1 < argc) {
            surfId = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--meters") == 0) {
            scaleFactor = 1.0f;
            scaleSet = true;
        } else if (strcmp(argv[i], "--millimeters") == 0) {
            scaleFactor = 0.001f;
            scaleSet = true;
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    if (inputFile.isEmpty()) {
        fprintf(stderr, "No input file specified.\n");
        usage(argv[0]);
        return 1;
    }
    if (outputFile.isEmpty()) {
        fprintf(stderr, "No output file specified.\n");
        usage(argv[0]);
        return 1;
    }
    if (outputFormat == FORMAT_UNKNOWN) {
        fprintf(stderr, "Cannot determine output format. Use --outtri, --outfif, --outsurf, or --outsmf.\n");
        return 1;
    }

    // Auto-detect input format if not specified
    if (inputFormat == FORMAT_UNKNOWN)
        inputFormat = detectFormat(inputFile);

    //
    // Read input surface
    //
    MatrixX3f rr;
    MatrixX3i tris;
    bool ok = false;

    switch (inputFormat) {
    case FORMAT_FIF:
        ok = readFifSurface(inputFile, rr, tris);
        break;
    case FORMAT_TRI:
        ok = readTriFile(inputFile, rr, tris);
        if (ok && !scaleSet)
            scaleFactor = 0.001f; // tri files are typically in mm
        break;
    case FORMAT_FREESURFER: {
        FsSurface surface;
        ok = FsSurface::read(inputFile, surface);
        if (ok) {
            rr = surface.rr();
            tris = surface.tris();
        }
        break;
    }
    case FORMAT_SMF:
        ok = readSmfFile(inputFile, rr, tris);
        if (ok && !scaleSet)
            scaleFactor = 0.001f; // smf files are typically in mm
        break;
    default:
        fprintf(stderr, "Cannot determine input format. Use --surf, --fif, --tri, or --smf.\n");
        return 1;
    }

    if (!ok) {
        fprintf(stderr, "Failed to read input surface.\n");
        return 1;
    }

    printf("Read surface: %ld vertices, %ld triangles\n",
           static_cast<long>(rr.rows()), static_cast<long>(tris.rows()));

    // Apply scale factor to convert to meters (internal representation)
    if (scaleFactor != 1.0f) {
        rr *= scaleFactor;
    }

    //
    // Write output surface
    //
    switch (outputFormat) {
    case FORMAT_FIF:
        ok = writeFifSurface(outputFile, rr, tris, surfId);
        break;
    case FORMAT_TRI:
        // tri format uses mm
        ok = writeTriFile(outputFile, rr * 1000.0f, tris);
        break;
    case FORMAT_FREESURFER:
        fprintf(stderr, "FreeSurfer surface writing not yet supported. Use --outtri or --outfif.\n");
        return 1;
    case FORMAT_SMF:
        // smf format uses mm
        ok = writeSmfFile(outputFile, rr * 1000.0f, tris);
        break;
    default:
        fprintf(stderr, "Unknown output format.\n");
        return 1;
    }

    if (!ok) {
        fprintf(stderr, "Failed to write output surface.\n");
        return 1;
    }

    printf("Wrote surface to %s\n", outputFile.toUtf8().constData());
    return 0;
}
