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
 * @brief    Create a volumetric source space inside a BEM surface.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_source_space.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_constants.h>

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
    fprintf(stderr, "Create a volumetric source space inside a BEM surface.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --bem <file>       BEM FIFF file (inner skull surface used as boundary)\n");
    fprintf(stderr, "  --grid <mm>        Grid spacing in mm (default: 7.0)\n");
    fprintf(stderr, "  --exclude <mm>     Exclude points closer to CM than this (default: 0)\n");
    fprintf(stderr, "  --mindist <mm>     Minimum distance from surface in mm (default: 5.0)\n");
    fprintf(stderr, "  --out <file>       Output source space FIFF file\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString bemFile;
    QString outFile;
    float gridMm = 7.0f;
    float excludeMm = 0.0f;
    float mindistMm = 5.0f;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--bem") == 0) {
            if (++k >= argc) { qCritical("--bem: argument required."); return 1; }
            bemFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--grid") == 0) {
            if (++k >= argc) { qCritical("--grid: argument required."); return 1; }
            gridMm = atof(argv[k]);
        }
        else if (strcmp(argv[k], "--exclude") == 0) {
            if (++k >= argc) { qCritical("--exclude: argument required."); return 1; }
            excludeMm = atof(argv[k]);
        }
        else if (strcmp(argv[k], "--mindist") == 0) {
            if (++k >= argc) { qCritical("--mindist: argument required."); return 1; }
            mindistMm = atof(argv[k]);
        }
        else if (strcmp(argv[k], "--out") == 0) {
            if (++k >= argc) { qCritical("--out: argument required."); return 1; }
            outFile = QString(argv[k]);
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (bemFile.isEmpty()) { qCritical("--bem is required."); usage(argv[0]); return 1; }
    if (outFile.isEmpty()) { qCritical("--out is required."); usage(argv[0]); return 1; }

    // Convert mm to meters
    float grid = gridMm / 1000.0f;
    float exclude = excludeMm / 1000.0f;
    float mindist = mindistMm / 1000.0f;

    // Read BEM file - find innermost surface
    QFile fBem(bemFile);
    MNEBem bem(fBem);
    if (bem.size() == 0) {
        qCritical("Cannot read BEM from: %s", qPrintable(bemFile));
        return 1;
    }

    // Find inner skull surface (smallest = innermost)
    int innerIdx = 0;
    for (int i = 1; i < bem.size(); ++i) {
        if (bem[i].id == FIFFV_BEM_SURF_ID_BRAIN) {
            innerIdx = i;
            break;
        }
    }

    const MNEBemSurface& innerSurf = bem[innerIdx];
    printf("Using surface: %s (%d vertices, %d triangles)\n",
           qPrintable(MNEBemSurface::id_name(innerSurf.id)),
           innerSurf.np, innerSurf.ntri);

    // Create MNESurface from BEM surface for the library function
    MNESurface surf;
    surf.np = innerSurf.np;
    surf.ntri = innerSurf.ntri;
    surf.rr = innerSurf.rr;
    surf.nn = innerSurf.nn;
    surf.itris = innerSurf.itris;
    surf.id = innerSurf.id;
    surf.coord_frame = innerSurf.coord_frame;

    printf("Creating volume source space (grid=%.1f mm, mindist=%.1f mm, exclude=%.1f mm)...\n",
           gridMm, mindistMm, excludeMm);

    // Use the existing library function
    MNESourceSpace* sp = MNESourceSpace::make_volume_source_space(surf, grid, exclude, mindist);
    if (!sp) {
        qCritical("Volume source space creation failed.");
        return 1;
    }

    printf("Volume source space: %d total points, %d active\n", sp->np, sp->nuse);

    // Write output
    QFile outF(outFile);
    outF.open(QIODevice::WriteOnly);
    FiffStream::SPtr outStream = FiffStream::start_file(outF);
    if (!outStream) {
        qCritical("Cannot write output file: %s", qPrintable(outFile));
        delete sp;
        return 1;
    }
    outStream->start_block(FIFFB_MNE);
    outStream->start_block(FIFFB_MNE_SOURCE_SPACE);
    sp->writeToStream(outStream, false);
    outStream->end_block(FIFFB_MNE_SOURCE_SPACE);
    outStream->end_block(FIFFB_MNE);
    outStream->end_file();
    outF.close();
    printf("Written volume source space to: %s\n", qPrintable(outFile));

    delete sp;
    return 0;
}
