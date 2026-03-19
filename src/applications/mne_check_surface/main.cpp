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
 * @brief    Check BEM surface topology (completeness, nesting, thickness).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem.h>
#include <mne/mne_bem_surface.h>
#include <fiff/fiff_constants.h>
#include <fs/fs_surface.h>

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
#include <Eigen/Geometry>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace FSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE CONSTANTS
//=============================================================================================================

#define PROGRAM_VERSION "2.0.0"

//=============================================================================================================
// STATIC HELPERS (ported from SurfaceChecks in mne_surf2bem)
//=============================================================================================================

static QString nameOf(int id)
{
    switch (id) {
        case FIFFV_BEM_SURF_ID_HEAD:  return "outer skin ";
        case FIFFV_BEM_SURF_ID_SKULL: return "outer skull";
        case FIFFV_BEM_SURF_ID_BRAIN: return "inner skull";
        default:                      return "unknown    ";
    }
}

//=============================================================================================================

static double solidAngle(const Vector3f& from,
                          const Vector3f& v0,
                          const Vector3f& v1,
                          const Vector3f& v2)
{
    Vector3d d1 = (v0 - from).cast<double>();
    Vector3d d2 = (v1 - from).cast<double>();
    Vector3d d3 = (v2 - from).cast<double>();

    double l1 = d1.norm();
    double l2 = d2.norm();
    double l3 = d3.norm();

    double triple = d1.cross(d2).dot(d3);
    double s = l1 * l2 * l3
             + d1.dot(d2) * l3
             + d1.dot(d3) * l2
             + d2.dot(d3) * l1;

    return 2.0 * atan2(triple, s);
}

//=============================================================================================================

static double sumSolids(const Vector3f& from, const MNEBemSurface& surf)
{
    double totAngle = 0.0;
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.itris(k, 0));
        Vector3f v1 = surf.rr.row(surf.itris(k, 1));
        Vector3f v2 = surf.rr.row(surf.itris(k, 2));
        totAngle += solidAngle(from, v0, v1, v2);
    }
    return totAngle;
}

//=============================================================================================================

static bool isCompleteSurface(const MNEBemSurface& surf)
{
    Vector3f cm = Vector3f::Zero();
    for (int k = 0; k < surf.np; ++k)
        cm += surf.rr.row(k).transpose();
    cm /= static_cast<float>(surf.np);

    printf("%s CM is %6.2f %6.2f %6.2f mm\n",
           qPrintable(nameOf(surf.id)),
           1000.0f * cm(0), 1000.0f * cm(1), 1000.0f * cm(2));

    double totAngle = sumSolids(cm, surf) / (4.0 * M_PI);
    if (fabs(totAngle - 1.0) > 1e-5) {
        printf("Surface %s is NOT complete (solid angle = %g * 4*PI instead of 1.0)\n",
               qPrintable(nameOf(surf.id)), totAngle);
        return false;
    }
    printf("Surface %s is complete.\n", qPrintable(nameOf(surf.id)));
    return true;
}

//=============================================================================================================

static bool checkNesting(const QVector<MNEBemSurface>& surfs)
{
    for (int j = 0; j < surfs.size() - 1; ++j) {
        printf("Checking that %s is inside %s...",
               qPrintable(nameOf(surfs[j + 1].id)),
               qPrintable(nameOf(surfs[j].id)));

        // Test first vertex of inner surface against outer
        Vector3f pt = surfs[j + 1].rr.row(0);
        double totAngle = sumSolids(pt, surfs[j]) / (4.0 * M_PI);
        if (fabs(totAngle - 1.0) > 1e-5) {
            printf("[FAILED]\n");
            return false;
        }
        printf("[OK]\n");
    }
    return true;
}

//=============================================================================================================

static void reportTriangleAreas(const MNEBemSurface& surf, const QString& name)
{
    double minArea = 1e30, maxArea = 0.0, totalArea = 0.0;
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.itris(k, 0));
        Vector3f v1 = surf.rr.row(surf.itris(k, 1));
        Vector3f v2 = surf.rr.row(surf.itris(k, 2));
        double area = 0.5 * ((v1 - v0).cross(v2 - v0)).cast<double>().norm();
        if (area < minArea) minArea = area;
        if (area > maxArea) maxArea = area;
        totalArea += area;
    }
    printf("%s: %d triangles, area %7.1f ... %7.1f mm^2 (total %10.1f mm^2)\n",
           qPrintable(name), surf.ntri,
           1e6 * minArea, 1e6 * maxArea, 1e6 * totalArea);
}

//=============================================================================================================

static float minSurfaceDist(const MNEBemSurface& s1, const MNEBemSurface& s2)
{
    float minDist = 1e30f;
    for (int k1 = 0; k1 < s1.np; ++k1) {
        for (int k2 = 0; k2 < s2.np; ++k2) {
            float dist = (s1.rr.row(k1) - s2.rr.row(k2)).norm();
            if (dist < minDist) minDist = dist;
        }
    }
    return minDist;
}

//=============================================================================================================
// USAGE
//=============================================================================================================

static void usage(const char *name)
{
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "Check BEM surface topology.\n\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --bem <file>       BEM FIFF file to check\n");
    fprintf(stderr, "  --surf <file>      FreeSurfer surface file to check (standalone)\n");
    fprintf(stderr, "  --id <id>          Surface ID for --surf: 4=head, 3=skull, 1=brain (default: 1)\n");
    fprintf(stderr, "  --thickness        Also check inter-surface distances\n");
    fprintf(stderr, "  --help             Print this help\n");
    fprintf(stderr, "  --version          Print version\n");
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QString bemFile;
    QString surfFile;
    int surfId = FIFFV_BEM_SURF_ID_BRAIN;
    bool doThickness = false;

    for (int k = 1; k < argc; k++) {
        if (strcmp(argv[k], "--help") == 0) { usage(argv[0]); return 0; }
        else if (strcmp(argv[k], "--version") == 0) { printf("%s version %s\n", argv[0], PROGRAM_VERSION); return 0; }
        else if (strcmp(argv[k], "--bem") == 0) {
            if (++k >= argc) { qCritical("--bem: argument required."); return 1; }
            bemFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--surf") == 0) {
            if (++k >= argc) { qCritical("--surf: argument required."); return 1; }
            surfFile = QString(argv[k]);
        }
        else if (strcmp(argv[k], "--id") == 0) {
            if (++k >= argc) { qCritical("--id: argument required."); return 1; }
            surfId = atoi(argv[k]);
        }
        else if (strcmp(argv[k], "--thickness") == 0) { doThickness = true; }
        else { qCritical("Unrecognized option: %s", argv[k]); usage(argv[0]); return 1; }
    }

    if (bemFile.isEmpty() && surfFile.isEmpty()) {
        qCritical("Either --bem or --surf is required.");
        usage(argv[0]);
        return 1;
    }

    QVector<MNEBemSurface> surfs;

    if (!bemFile.isEmpty()) {
        // Read BEM from FIFF
        QFile file(bemFile);
        MNEBem bem(file);
        if (bem.size() == 0) {
            qCritical("Cannot read BEM from: %s", qPrintable(bemFile));
            return 1;
        }
        printf("Read %d surfaces from %s\n\n", bem.size(), qPrintable(bemFile));
        for (int i = 0; i < bem.size(); ++i)
            surfs.append(bem[i]);
    } else {
        // Read single FreeSurfer surface
        FsSurface surface;
        if (!FsSurface::read(surfFile, surface)) {
            qCritical("Cannot read surface: %s", qPrintable(surfFile));
            return 1;
        }
        MNEBemSurface bemSurf;
        bemSurf.id = surfId;
        bemSurf.np = surface.rr().rows();
        bemSurf.ntri = surface.tris().rows();
        bemSurf.rr = surface.rr();
        bemSurf.itris = surface.tris();
        bemSurf.nn = surface.nn();
        bemSurf.coord_frame = FIFFV_COORD_MRI;
        surfs.append(bemSurf);
        printf("Read surface: %s (%d vertices, %d triangles)\n\n",
               qPrintable(surfFile), bemSurf.np, bemSurf.ntri);
    }

    // Check completeness
    bool allOk = true;
    for (int k = 0; k < surfs.size(); ++k) {
        if (!isCompleteSurface(surfs[k]))
            allOk = false;
        reportTriangleAreas(surfs[k], nameOf(surfs[k].id));
    }
    printf("\n");

    // Check nesting if multiple surfaces
    if (surfs.size() > 1) {
        if (!checkNesting(surfs))
            allOk = false;
    }

    // Check inter-surface distances
    if (doThickness && surfs.size() > 1) {
        printf("\n");
        for (int k = 0; k < surfs.size() - 1; ++k) {
            float dist = minSurfaceDist(surfs[k], surfs[k + 1]);
            printf("Minimum distance between %s and %s: %6.1f mm\n",
                   qPrintable(nameOf(surfs[k].id)),
                   qPrintable(nameOf(surfs[k + 1].id)),
                   1000.0f * dist);
        }
    }

    if (allOk)
        printf("\nAll surface checks passed.\n");
    else
        printf("\nSome surface checks FAILED.\n");

    return allOk ? 0 : 1;
}
