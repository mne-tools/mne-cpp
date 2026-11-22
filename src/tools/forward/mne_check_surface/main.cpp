//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     main.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 * @brief    Check BEM surface topology (completeness, nesting, thickness).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne/mne_bem.h>
#include <mne/mne/mne_bem_surface.h>
#include <mne/fiff/fiff_constants.h>
#include <mne/fs/fs_surface.h>
#include <mne/base/log.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Geometry>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <cmath>
#include <numbers>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE CONSTANTS
//=============================================================================================================

#define PROGRAM_VERSION MNE_CPP_VERSION

//=============================================================================================================
// STATIC HELPERS (ported from SurfaceChecks in mne_surf2bem)
//=============================================================================================================

static QString nameOf(int id)
{
    switch (id) {
        case FIFFV_BEM_SURF_ID_HEAD:
            return "outer skin ";
        case FIFFV_BEM_SURF_ID_SKULL:
            return "outer skull";
        case FIFFV_BEM_SURF_ID_BRAIN:
            return "inner skull";
        default:
            return "unknown    ";
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
    double s = l1 * l2 * l3 + d1.dot(d2) * l3 + d1.dot(d3) * l2 + d2.dot(d3) * l1;

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

    Log::info("{} CM is {:6.2f} {:6.2f} {:6.2f} mm", nameOf(surf.id).toStdString(), 1000.0f * cm(0), 1000.0f * cm(1), 1000.0f * cm(2));

    double totAngle = sumSolids(cm, surf) / (4.0 * std::numbers::pi);
    if (fabs(totAngle - 1.0) > 1e-5) {
        Log::info("Surface {} is NOT complete (solid angle = {:g} * 4*PI instead of 1.0)", nameOf(surf.id).toStdString(), totAngle);
        return false;
    }
    Log::info("Surface {} is complete.", nameOf(surf.id).toStdString());
    return true;
}

//=============================================================================================================

static bool checkNesting(const QVector<MNEBemSurface>& surfs)
{
    for (int j = 0; j < surfs.size() - 1; ++j) {
        const QString inner = nameOf(surfs[j + 1].id);
        const QString outer = nameOf(surfs[j].id);

        // Test first vertex of inner surface against outer
        Vector3f pt = surfs[j + 1].rr.row(0);
        double totAngle = sumSolids(pt, surfs[j]) / (4.0 * std::numbers::pi);
        if (fabs(totAngle - 1.0) > 1e-5) {
            Log::info("Checking that {} is inside {}... [FAILED]", inner.toStdString(), outer.toStdString());
            return false;
        }
        Log::info("Checking that {} is inside {}... [OK]", inner.toStdString(), outer.toStdString());
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
        if (area < minArea)
            minArea = area;
        if (area > maxArea)
            maxArea = area;
        totalArea += area;
    }
    Log::info("{}: {} triangles, area {:7.1f} ... {:7.1f} mm^2 (total {:10.1f} mm^2)", name.toStdString(), surf.ntri, 1e6 * minArea, 1e6 * maxArea, 1e6 * totalArea);
}

//=============================================================================================================

static float minSurfaceDist(const MNEBemSurface& s1, const MNEBemSurface& s2)
{
    float minDist = 1e30f;
    for (int k1 = 0; k1 < s1.np; ++k1) {
        for (int k2 = 0; k2 < s2.np; ++k2) {
            float dist = (s1.rr.row(k1) - s2.rr.row(k2)).norm();
            if (dist < minDist)
                minDist = dist;
        }
    }
    return minDist;
}

//=============================================================================================================
// MAIN
//=============================================================================================================

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("mne_check_surface");
    QCoreApplication::setApplicationVersion(PROGRAM_VERSION);

    QCommandLineParser parser;
    parser.setApplicationDescription("Check BEM surface topology (completeness, nesting, thickness).\n\nUse --bem for BEM FIFF files or --surf for FreeSurfer surface files.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bemOpt("bem", "BEM FIFF file to check.", "file");
    parser.addOption(bemOpt);

    QCommandLineOption surfOpt("surf", "FreeSurfer surface file to check (standalone).", "file");
    parser.addOption(surfOpt);

    QCommandLineOption idOpt("id", "Surface ID for --surf: 4=head, 3=skull, 1=brain.", "id", "1");
    parser.addOption(idOpt);

    QCommandLineOption thicknessOpt("thickness", "Also check inter-surface distances.");
    parser.addOption(thicknessOpt);

    parser.process(app);

    QString bemFile = parser.value(bemOpt);
    QString surfFile = parser.value(surfOpt);
    int surfId = parser.value(idOpt).toInt();
    bool doThickness = parser.isSet(thicknessOpt);

    if (bemFile.isEmpty() && surfFile.isEmpty()) {
        Log::critical("Either --bem or --surf is required.");
        return 1;
    }

    QVector<MNEBemSurface> surfs;

    if (!bemFile.isEmpty()) {
        // Read BEM from FIFF
        MNEBem bem(bemFile.toStdU16String());
        if (bem.size() == 0) {
            Log::critical("Cannot read BEM from: {}", bemFile.toStdString());
            return 1;
        }
        Log::info("Read {} surfaces from {}\n", bem.size(), bemFile.toStdString());
        for (int i = 0; i < std::ssize(bem); ++i)
            surfs.append(bem[i]);
    } else {
        // Read single FreeSurfer surface
        FsSurface surface;
        if (!FsSurface::read(surfFile.toStdU16String(), surface)) {
            Log::critical("Cannot read surface: {}", surfFile.toStdString());
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
        Log::info("Read surface: {} ({} vertices, {} triangles)\n", surfFile.toStdString(), bemSurf.np, bemSurf.ntri);
    }

    // Check completeness
    bool allOk = true;
    for (int k = 0; k < surfs.size(); ++k) {
        if (!isCompleteSurface(surfs[k]))
            allOk = false;
        reportTriangleAreas(surfs[k], nameOf(surfs[k].id));
    }
    Log::info("{}", "");

    // Check nesting if multiple surfaces
    if (surfs.size() > 1) {
        if (!checkNesting(surfs))
            allOk = false;
    }

    // Check inter-surface distances
    if (doThickness && surfs.size() > 1) {
        Log::info("{}", "");
        for (int k = 0; k < surfs.size() - 1; ++k) {
            float dist = minSurfaceDist(surfs[k], surfs[k + 1]);
            Log::info("Minimum distance between {} and {}: {:6.1f} mm", nameOf(surfs[k].id).toStdString(), nameOf(surfs[k + 1].id).toStdString(), 1000.0f * dist);
        }
    }

    if (allOk)
        Log::info("\nAll surface checks passed.");
    else
        Log::info("\nSome surface checks FAILED.");

    return allOk ? 0 : 1;
}
