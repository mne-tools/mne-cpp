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

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
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
using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// STATIC DEFINITIONS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================

int main(int argc, char *argv[])
{
    qInstallMessageHandler(MNELogger::customLogWriter);
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_list_bem");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Produce ASCII data from a BEM FIFF file.\n\nLists vertices and triangles of specified BEM surfaces.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bemOpt("bem", "BEM model file.", "name");
    parser.addOption(bemOpt);

    QCommandLineOption outOpt("out", "Output text file.", "name");
    parser.addOption(outOpt);

    QCommandLineOption idOpt("id", "Surface id to list.", "id", "4");
    parser.addOption(idOpt);

    QCommandLineOption gdipoliOpt("gdipoli", "Oostendorp format (vertices + triangles).");
    parser.addOption(gdipoliOpt);

    QCommandLineOption xfitOpt("xfit", "xfit format (same as --gdipoli --meters).");
    parser.addOption(xfitOpt);

    QCommandLineOption oldOpt("old", "Old stream format.");
    parser.addOption(oldOpt);

    QCommandLineOption metersOpt("meters", "Output in meters (default: millimeters).");
    parser.addOption(metersOpt);

    parser.process(app);

    QString bemName = parser.value(bemOpt);
    QString outName = parser.value(outOpt);
    int surfId = parser.value(idOpt).toInt();
    bool thomOutput = false;
    bool oldOutput = false;
    float lengthMult = 1000.0f;

    if (parser.isSet(gdipoliOpt)) {
        thomOutput = true;
        oldOutput = false;
    }
    if (parser.isSet(xfitOpt)) {
        thomOutput = true;
        oldOutput = false;
        lengthMult = 1.0f;
    }
    if (parser.isSet(oldOpt)) {
        thomOutput = false;
        oldOutput = true;
    }
    if (parser.isSet(metersOpt)) {
        lengthMult = 1.0f;
    }

    if (bemName.isEmpty() || outName.isEmpty()) {
        qCritical("Both --bem and --out are required.");
        parser.showHelp(1);
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
