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
 * @brief    List source space information.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_spaces.h>
#include <mne/mne_source_space.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_coord_trans.h>
#include <fiff/fiff_types.h>

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

using namespace MNELIB;
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
    fprintf(stderr, "List source space information.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --src <file>    Source space FIFF file\n");
    fprintf(stderr, "  --pnt <file>    Output point file (MRIlab format)\n");
    fprintf(stderr, "  --dip <file>    Output dipole file (MRIlab format)\n");
    fprintf(stderr, "  --vert <file>   Output vertex file (text)\n");
    fprintf(stderr, "  --all           List all vertices (not just active)\n");
    fprintf(stderr, "  --coord <name>  Coordinate frame: head or mri (default: mri)\n");
    fprintf(stderr, "  --help          Print this help\n");
    fprintf(stderr, "  --version       Print version\n");
}

//=============================================================================================================

static const char *coordFrameName(int frame)
{
    switch (frame) {
    case FIFFV_COORD_MRI:     return "MRI (surface RAS)";
    case FIFFV_COORD_HEAD:    return "Head";
    case FIFFV_COORD_DEVICE:  return "Device";
    case FIFFV_COORD_UNKNOWN: return "Unknown";
    default: return "Other";
    }
}

//=============================================================================================================

static const char *spaceTypeName(int type)
{
    switch (type) {
    case FIFFV_MNE_SPACE_SURFACE: return "Surface";
    case FIFFV_MNE_SPACE_VOLUME:  return "Volume";
    case FIFFV_MNE_SPACE_DISCRETE: return "Discrete";
    default: return "Unknown";
    }
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString srcName;
    QString pntName;
    QString dipName;
    QString vertName;
    bool listAll = false;
    int coordFrame = FIFFV_COORD_MRI;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        }
        else if (strcmp(argv[k], "--src") == 0) {
            if (++k >= argc) { qCritical("--src: argument required."); return 1; }
            srcName = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--pnt") == 0) {
            if (++k >= argc) { qCritical("--pnt: argument required."); return 1; }
            pntName = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--dip") == 0) {
            if (++k >= argc) { qCritical("--dip: argument required."); return 1; }
            dipName = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--vert") == 0) {
            if (++k >= argc) { qCritical("--vert: argument required."); return 1; }
            vertName = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--all") == 0) { listAll = true; }
        else if (strcmp(argv[k], "--coord") == 0) {
            if (++k >= argc) { qCritical("--coord: argument required."); return 1; }
            if (strcmp(argv[k], "head") == 0) coordFrame = FIFFV_COORD_HEAD;
            else if (strcmp(argv[k], "mri") == 0) coordFrame = FIFFV_COORD_MRI;
            else { qCritical("Unknown coordinate frame: %s (use head or mri)", argv[k]); return 1; }
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (srcName.isEmpty()) {
        qCritical("--src is required.");
        usage(argv[0]);
        return 1;
    }

    // Read source spaces
    QFile srcFile(srcName);
    FiffStream::SPtr stream(new FiffStream(&srcFile));
    if (!stream->open()) {
        qCritical("Cannot open source space file: %s", qPrintable(srcName));
        return 1;
    }

    MNESourceSpaces sourceSpaces;
    if (!MNESourceSpaces::readFromStream(stream, true, sourceSpaces)) {
        qCritical("Cannot read source spaces from file.");
        return 1;
    }
    stream->close();

    if (sourceSpaces.size() == 0) {
        qCritical("No source spaces found in file.");
        return 1;
    }

    // Print summary
    printf("Source space file : %s\n", qPrintable(srcName));
    printf("Number of spaces  : %d\n", sourceSpaces.size());
    printf("\n");

    int totalActive = 0;
    int totalVerts = 0;

    for (int s = 0; s < sourceSpaces.size(); s++) {
        const MNESourceSpace &space = sourceSpaces[s];

        int nvert = space.np;
        int nuse = space.nuse;
        int ntri = space.ntri;
        totalActive += nuse;
        totalVerts += nvert;

        printf("--- Space %d ---\n", s + 1);
        printf("  Type            : %s\n", spaceTypeName(space.type));
        printf("  Vertices        : %d\n", nvert);
        printf("  Active vertices : %d\n", nuse);
        printf("  Triangles       : %d\n", ntri);
        printf("  Coord frame     : %s\n", coordFrameName(space.coord_frame));

        if (space.id == FIFFV_MNE_SURF_LEFT_HEMI)
            printf("  Hemisphere      : Left\n");
        else if (space.id == FIFFV_MNE_SURF_RIGHT_HEMI)
            printf("  Hemisphere      : Right\n");

        if (nuse > 0 && space.rr.rows() > 0) {
            // Compute bounding box of active vertices
            float xmin = 1e10, xmax = -1e10;
            float ymin = 1e10, ymax = -1e10;
            float zmin = 1e10, zmax = -1e10;

            for (int v = 0; v < nvert; v++) {
                if (!listAll && space.inuse(v) == 0)
                    continue;
                float x = space.rr(v, 0) * 1000.0f;
                float y = space.rr(v, 1) * 1000.0f;
                float z = space.rr(v, 2) * 1000.0f;
                if (x < xmin) xmin = x;
                if (x > xmax) xmax = x;
                if (y < ymin) ymin = y;
                if (y > ymax) ymax = y;
                if (z < zmin) zmin = z;
                if (z > zmax) zmax = z;
            }
            printf("  Extent (mm)     : x [%.1f, %.1f] y [%.1f, %.1f] z [%.1f, %.1f]\n",
                   xmin, xmax, ymin, ymax, zmin, zmax);
        }
        printf("\n");
    }

    printf("Total active vertices : %d / %d\n", totalActive, totalVerts);

    // Write point file (MRIlab format)
    if (!pntName.isEmpty()) {
        QFile pntFile(pntName);
        if (!pntFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open output file: %s", qPrintable(pntName));
            return 1;
        }
        QTextStream pnt(&pntFile);

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace &space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0) continue;
                pnt << QString::asprintf("%10.4f %10.4f %10.4f\n",
                    space.rr(v, 0) * 1000.0f,
                    space.rr(v, 1) * 1000.0f,
                    space.rr(v, 2) * 1000.0f);
            }
        }
        printf("Wrote point file: %s\n", qPrintable(pntName));
    }

    // Write dipole file (MRIlab format: position + orientation)
    if (!dipName.isEmpty()) {
        QFile dipFile(dipName);
        if (!dipFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open output file: %s", qPrintable(dipName));
            return 1;
        }
        QTextStream dip(&dipFile);

        int dipNum = 1;
        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace &space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0) continue;
                dip << QString::asprintf("%5d %8.3f %8.3f %8.3f  %7.4f %7.4f %7.4f\n",
                    dipNum++,
                    space.rr(v, 0) * 1000.0f, space.rr(v, 1) * 1000.0f, space.rr(v, 2) * 1000.0f,
                    space.nn(v, 0), space.nn(v, 1), space.nn(v, 2));
            }
        }
        printf("Wrote dipole file: %s\n", qPrintable(dipName));
    }

    // Write vertex file (text: vertex_number x y z)
    if (!vertName.isEmpty()) {
        QFile vertFile(vertName);
        if (!vertFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qCritical("Cannot open output file: %s", qPrintable(vertName));
            return 1;
        }
        QTextStream vert(&vertFile);

        for (int s = 0; s < sourceSpaces.size(); s++) {
            const MNESourceSpace &space = sourceSpaces[s];
            for (int v = 0; v < space.np; v++) {
                if (!listAll && space.inuse(v) == 0) continue;
                vert << QString::asprintf("%6d %10.4f %10.4f %10.4f\n",
                    v,
                    space.rr(v, 0) * 1000.0f, space.rr(v, 1) * 1000.0f, space.rr(v, 2) * 1000.0f);
            }
        }
        printf("Wrote vertex file: %s\n", qPrintable(vertName));
    }

    return 0;
}
