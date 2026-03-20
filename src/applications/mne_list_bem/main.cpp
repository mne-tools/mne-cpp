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
 * @brief    Export BEM surface data to ASCII text or FreeSurfer format.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>

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
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Produces ASCII data from a BEM FIFF file.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --bem <name>    BEM model file\n");
    fprintf(stderr, "  --out <name>    Output text file\n");
    fprintf(stderr, "  --id <id>       Surface id to list (default: %d)\n", FIFFV_BEM_SURF_ID_HEAD);
    fprintf(stderr, "                  %d = outer skin (scalp)\n", FIFFV_BEM_SURF_ID_HEAD);
    fprintf(stderr, "                  %d = outer skull\n", FIFFV_BEM_SURF_ID_SKULL);
    fprintf(stderr, "                  %d = inner skull\n", FIFFV_BEM_SURF_ID_BRAIN);
    fprintf(stderr, "  --gdipoli       Oostendorp format (vertices + triangles)\n");
    fprintf(stderr, "  --xfit          xfit format (same as --gdipoli --meters)\n");
    fprintf(stderr, "  --old           Old stream format\n");
    fprintf(stderr, "  --meters        Output in meters (default: millimeters)\n");
    fprintf(stderr, "  --help          Print this help\n");
    fprintf(stderr, "  --version       Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString bemName;
    QString outName;
    int surfId = FIFFV_BEM_SURF_ID_HEAD;
    bool thomOutput = false;
    bool oldOutput = false;
    float lengthMult = 1000.0f;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else if (strcmp(argv[k], "--version") == 0) {
            fprintf(stderr, "%s version %s\n", argv[0], PROGRAM_VERSION);
            return 0;
        } else if (strcmp(argv[k], "--bem") == 0) {
            if (k + 1 >= argc) { qCritical("--bem: argument required."); return 1; }
            bemName = QString(argv[++k]);
        } else if (strcmp(argv[k], "--out") == 0) {
            if (k + 1 >= argc) { qCritical("--out: argument required."); return 1; }
            outName = QString(argv[++k]);
        } else if (strcmp(argv[k], "--id") == 0) {
            if (k + 1 >= argc) { qCritical("--id: argument required."); return 1; }
            surfId = atoi(argv[++k]);
        } else if (strcmp(argv[k], "--gdipoli") == 0) {
            thomOutput = true;
            oldOutput = false;
        } else if (strcmp(argv[k], "--xfit") == 0) {
            thomOutput = true;
            oldOutput = false;
            lengthMult = 1.0f;
        } else if (strcmp(argv[k], "--old") == 0) {
            thomOutput = false;
            oldOutput = true;
        } else if (strcmp(argv[k], "--meters") == 0) {
            lengthMult = 1.0f;
        } else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (bemName.isEmpty() || outName.isEmpty()) {
        qCritical("Both --bem and --out are required.");
        usage(argv[0]);
        return 1;
    }

    fprintf(stderr, "input  file : %s\n", qPrintable(bemName));
    fprintf(stderr, "output file : %s\n", qPrintable(outName));
    fprintf(stderr, "surface id  : %d\n", surfId);
    if (lengthMult == 1.0f)
        fprintf(stderr, "output in meters.\n");
    else
        fprintf(stderr, "output in millimeters.\n");
    if (thomOutput)
        fprintf(stderr, "output in Oostendorp (xfit) format.\n");
    else if (oldOutput)
        fprintf(stderr, "output in old stream format.\n");
    else
        fprintf(stderr, "output the vertex coordinates only.\n");

    // Read BEM surfaces
    QFile bemFile(bemName);
    FiffStream::SPtr stream(new FiffStream(&bemFile));
    if (!stream->open()) {
        qCritical("Cannot open BEM file: %s", qPrintable(bemName));
        return 1;
    }

    MNEBem bem;
    if (!MNEBem::readFromStream(stream, true, bem)) {
        qCritical("Cannot read BEM surfaces from: %s", qPrintable(bemName));
        return 1;
    }
    stream->close();

    // Find the requested surface
    int surfIdx = -1;
    for (int i = 0; i < bem.size(); i++) {
        if (bem[i].id == surfId) {
            surfIdx = i;
            break;
        }
    }
    if (surfIdx < 0) {
        qCritical("Surface id %d not found in %s", surfId, qPrintable(bemName));
        return 1;
    }

    const MNEBemSurface &surf = bem[surfIdx];

    // Write output
    FILE *out = fopen(qPrintable(outName), "w");
    if (!out) {
        qCritical("Cannot open output file: %s", qPrintable(outName));
        return 1;
    }

    if (oldOutput) {
        fprintf(out, "%5d\n", surf.np);
        for (int k = 0; k < surf.np; k++)
            fprintf(out, "%5d %9.4f %9.4f %9.4f\n",
                    k + 1,
                    lengthMult * surf.rr(k, 0),
                    lengthMult * surf.rr(k, 1),
                    lengthMult * surf.rr(k, 2));
        fprintf(out, "%5d\n", surf.ntri);
        for (int k = 0; k < surf.ntri; k++) {
            fprintf(out, "%5d %4d %4d %4d\n",
                    k + 1,
                    surf.itris(k, 0) + 1,
                    surf.itris(k, 2) + 1,
                    surf.itris(k, 1) + 1);
        }
    } else if (thomOutput) {
        fprintf(out, "%5d\n", surf.np);
        for (int k = 0; k < surf.np; k++)
            fprintf(out, "%5d %9.4f %9.4f %9.4f\n",
                    k + 1,
                    lengthMult * surf.rr(k, 0),
                    lengthMult * surf.rr(k, 1),
                    lengthMult * surf.rr(k, 2));
        fprintf(out, "%5d\n", surf.ntri);
        for (int k = 0; k < surf.ntri; k++) {
            fprintf(out, "%5d %4d %4d %4d\n",
                    k + 1,
                    surf.itris(k, 0) + 1,
                    surf.itris(k, 1) + 1,
                    surf.itris(k, 2) + 1);
        }
    } else {
        for (int k = 0; k < surf.np; k++)
            fprintf(out, "%10.3f %10.3f %10.3f\n",
                    lengthMult * surf.rr(k, 0),
                    lengthMult * surf.rr(k, 1),
                    lengthMult * surf.rr(k, 2));
    }

    fclose(out);
    fprintf(stderr, "done.\n");

    return 0;
}
