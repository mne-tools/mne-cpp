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
 * @brief    Prepare a BEM model for forward computation (compute BEM solution matrix).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fwd/fwd_bem_model.h>
#include <mne/mne_surface.h>
#include <fiff/fiff_stream.h>
#include <fiff/fiff_constants.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace FWDLIB;
using namespace FIFFLIB;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Compute the BEM solution matrix for forward modeling.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --bem <file>       Input BEM model FIFF file\n");
    fprintf(stderr, "  --sol <file>       Output solution FIFF file (default: derived from --bem)\n");
    fprintf(stderr, "  --method <name>    BEM method: linear or constant (default: linear)\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString bemFile;
    QString solFile;
    int bemMethod = FWD_BEM_LINEAR_COLL;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--bem") == 0) {
            if (++k >= argc) { qCritical("--bem: argument required."); return 1; }
            bemFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--sol") == 0) {
            if (++k >= argc) { qCritical("--sol: argument required."); return 1; }
            solFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--method") == 0) {
            if (++k >= argc) { qCritical("--method: argument required."); return 1; }
            QString method = QString(argv[k]).toLower();
            if (method == "linear")
                bemMethod = FWD_BEM_LINEAR_COLL;
            else if (method == "constant")
                bemMethod = FWD_BEM_CONSTANT_COLL;
            else {
                qCritical("Unknown method: %s (use 'linear' or 'constant')", argv[k]);
                return 1;
            }
        }
        else {
            qCritical("Unrecognized option: %s", argv[k]);
            usage(argv[0]);
            return 1;
        }
    }

    if (bemFile.isEmpty()) { qCritical("--bem is required."); usage(argv[0]); return 1; }

    // Derive solution filename  if not specified
    if (solFile.isEmpty())
        solFile = FwdBemModel::fwd_bem_make_bem_sol_name(bemFile);

    printf("BEM model file:    %s\n", qPrintable(bemFile));
    printf("BEM solution file: %s\n", qPrintable(solFile));
    printf("BEM method:        %s\n",
           qPrintable(FwdBemModel::fwd_bem_explain_method(bemMethod)));

    // Determine how many layers we have
    // Try three-layer first, fall back to single-layer
    FwdBemModel::UPtr bemModel;
    bemModel = FwdBemModel::fwd_bem_load_three_layer_surfaces(bemFile);
    if (!bemModel) {
        printf("Not a three-layer model, trying single-layer (homogeneous)...\n");
        bemModel = FwdBemModel::fwd_bem_load_homog_surface(bemFile);
    }

    if (!bemModel) {
        qCritical("Cannot load BEM surfaces from: %s", qPrintable(bemFile));
        return 1;
    }

    printf("Loaded %d BEM surface(s)\n", bemModel->nsurf);
    for (int k = 0; k < bemModel->nsurf; ++k) {
        printf("  Surface %d: %s  (%d vertices, %d triangles)\n",
               k + 1,
               qPrintable(FwdBemModel::fwd_bem_explain_surface(bemModel->surfs[k]->id)),
               bemModel->surfs[k]->np,
               bemModel->surfs[k]->ntri);
    }

    // Compute the BEM solution
    printf("\nComputing BEM solution...\n");
    if (bemModel->fwd_bem_compute_solution(bemMethod) != 0) {
        qCritical("BEM solution computation failed.");
        return 1;
    }
    printf("BEM solution computed successfully.\n");

    // Save the solution
    printf("Writing solution to: %s\n", qPrintable(solFile));

    QFile file(solFile);
    FiffStream::SPtr stream = FiffStream::start_file(file);
    if (!stream) {
        qCritical("Cannot open output file: %s", qPrintable(solFile));
        return 1;
    }

    stream->start_block(FIFFB_BEM);
    stream->write_int(FIFF_BEM_APPROX, &bemModel->bem_method);

    // Write solution as a float matrix
    stream->write_float_matrix(FIFF_MNE_FORWARD_SOLUTION, bemModel->solution);

    stream->end_block(FIFFB_BEM);
    stream->end_file();

    printf("BEM solution saved.\n");
    return 0;
}
