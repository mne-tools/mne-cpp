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
 * @brief    Fit a sphere to surface points (FreeSurfer or FIFF BEM surface).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <fiff/fiff_stream.h>
#include <fiff/fiff_tag.h>
#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fs/fs_surface.h>
#include <math/sphere.h>

#include <utils/generics/mne_logger.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
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
using namespace FSLIB;
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
    QCoreApplication::setApplicationName("mne_fit_sphere_to_surf");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Fit a sphere to surface points.\n\nReads a FreeSurfer surface or FIFF BEM surface and fits a best-fit sphere.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption surfOpt("surf", "Surface file (FreeSurfer format or FIFF BEM).", "name");
    parser.addOption(surfOpt);

    QCommandLineOption outOpt("out", "Output text file with sphere parameters.", "name");
    parser.addOption(outOpt);

    parser.process(app);

    QString surfName = parser.value(surfOpt);
    QString outName = parser.value(outOpt);

    if (surfName.isEmpty() || outName.isEmpty()) {
        qCritical("Both --surf and --out are required.");
        parser.showHelp(1);
    }

    MatrixX3f vertices;

    // Try reading as FIFF BEM first, then as FreeSurfer surface
    bool loaded = false;

    // Attempt FIFF BEM
    {
        QFile bemFile(surfName);
        FiffStream::SPtr stream(new FiffStream(&bemFile));
        if (stream->open()) {
            MNEBem bem;
            if (MNEBem::readFromStream(stream, true, bem) && bem.size() > 0) {
                vertices = bem[0].rr;
                fprintf(stderr, "Read FIFF BEM surface: %d vertices\n", static_cast<int>(vertices.rows()));
                loaded = true;
            }
            stream->close();
        }
    }

    // Attempt FreeSurfer surface
    if (!loaded) {
        FsSurface surf;
        if (FsSurface::read(surfName, surf, false)) {
            vertices = surf.rr();
            fprintf(stderr, "Read FreeSurfer surface: %d vertices\n", static_cast<int>(vertices.rows()));
            loaded = true;
        }
    }

    if (!loaded) {
        qCritical("Cannot read surface from: %s", qPrintable(surfName));
        return 1;
    }

    if (vertices.rows() < 4) {
        qCritical("Need at least 4 vertices to fit a sphere, got %d.", static_cast<int>(vertices.rows()));
        return 1;
    }

    // Fit sphere using simplex method
    fprintf(stderr, "Fitting sphere to %d points...\n", static_cast<int>(vertices.rows()));
    Sphere sphere = Sphere::fit_sphere_simplex(vertices);

    Vector3f center = sphere.center();
    float radius = sphere.radius();

    fprintf(stderr, "Sphere center : %10.6f %10.6f %10.6f m\n", center(0), center(1), center(2));
    fprintf(stderr, "Sphere radius : %10.6f m\n", radius);

    // Write output
    QFile outFile(outName);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical("Cannot open output file: %s", qPrintable(outName));
        return 1;
    }

    QTextStream out(&outFile);
    out.setRealNumberPrecision(8);
    out.setRealNumberNotation(QTextStream::FixedNotation);
    out << center(0) << " " << center(1) << " " << center(2) << " " << radius << "\n";
    outFile.close();

    fprintf(stderr, "Wrote sphere parameters to %s\n", qPrintable(outName));

    return 0;
}
