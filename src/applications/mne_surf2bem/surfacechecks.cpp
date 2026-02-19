//=============================================================================================================
/**
 * @file     surfacechecks.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     February, 2026
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
 *
 * @brief    SurfaceChecks class definition.
 *
 *           Ported from surface_checks.c in the original MNE C tool mne_surf2bem
 *           by Matti Hamalainen (SVN $Id: surface_checks.c 3186).
 *
 *           The solid angle computation uses van Oosterom's formula:
 *             omega = 2 * atan2(triple, s)
 *           where triple = (v1 x v2) . v3 and
 *             s = l1*l2*l3 + (v1.v2)*l3 + (v1.v3)*l2 + (v2.v3)*l1
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "surfacechecks.h"

#include <fiff/fiff_constants.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Geometry>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNESURF2BEM;
using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString SurfaceChecks::getNameOf(int id)
{
    switch (id) {
        case FIFFV_BEM_SURF_ID_HEAD:  return "outer skin ";
        case FIFFV_BEM_SURF_ID_SKULL: return "outer skull";
        case FIFFV_BEM_SURF_ID_BRAIN: return "inner skull";
        default:                      return "unknown    ";
    }
}

//=============================================================================================================

double SurfaceChecks::solidAngle(const Vector3f& from,
                                  const Vector3f& v0,
                                  const Vector3f& v1,
                                  const Vector3f& v2)
{
    //
    // Compute the solid angle according to van Oosterom's formula
    //
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

double SurfaceChecks::sumSolids(const Vector3f& from, const MNEBemSurface& surf)
{
    double totAngle = 0.0;
    for (int k = 0; k < surf.ntri; ++k) {
        Vector3f v0 = surf.rr.row(surf.tris(k, 0));
        Vector3f v1 = surf.rr.row(surf.tris(k, 1));
        Vector3f v2 = surf.rr.row(surf.tris(k, 2));
        totAngle += solidAngle(from, v0, v1, v2);
    }
    return totAngle;
}

//=============================================================================================================

bool SurfaceChecks::isCompleteSurface(const MNEBemSurface& surf)
{
    //
    // Compute center of mass
    //
    Vector3f cm = Vector3f::Zero();
    for (int k = 0; k < surf.np; ++k) {
        cm += surf.rr.row(k).transpose();
    }
    cm /= static_cast<float>(surf.np);

    fprintf(stderr, "%s CM is %6.2f %6.2f %6.2f mm\n",
            qPrintable(getNameOf(surf.id)),
            1000.0f * cm(0),
            1000.0f * cm(1),
            1000.0f * cm(2));

    //
    // Sum of solid angles from the center should be 4*pi for a complete surface
    //
    double totAngle = sumSolids(cm, surf) / (4.0 * M_PI);
    if (fabs(totAngle - 1.0) > 1e-5) {
        qCritical() << "Surface" << getNameOf(surf.id) << "is not complete"
                     << "(sum of solid angles =" << totAngle << "* 4*PI instead).";
        return false;
    }
    return true;
}

//=============================================================================================================

bool SurfaceChecks::isInside(const MNEBemSurface& from, const MNEBemSurface& to)
{
    for (int k = 0; k < from.np; ++k) {
        Vector3f pt = from.rr.row(k);
        double totAngle = sumSolids(pt, to) / (4.0 * M_PI);
        if (fabs(totAngle - 1.0) > 1e-5) {
            qCritical() << "Surface" << getNameOf(from.id)
                         << "is not completely inside surface" << getNameOf(to.id)
                         << "(sum of solid angles =" << totAngle << "* 4*PI).";
            return false;
        }
    }
    return true;
}

//=============================================================================================================

bool SurfaceChecks::checkSurfaces(const QVector<MNEBemSurface>& surfs)
{
    //
    // First check that surfaces are complete
    //
    for (int k = 0; k < surfs.size(); ++k) {
        if (!isCompleteSurface(surfs[k])) {
            return false;
        }
    }

    //
    // Then check the topology (each surface inside the previous one)
    //
    for (int j = 0; j < surfs.size() - 1; ++j) {
        fprintf(stderr, "Checking that %s surface is inside %s surface...",
                qPrintable(getNameOf(surfs[j + 1].id)),
                qPrintable(getNameOf(surfs[j].id)));

        if (!isInside(surfs[j + 1], surfs[j])) {
            fprintf(stderr, "[failed]\n");
            return false;
        }
        fprintf(stderr, "[ok]\n");
    }
    return true;
}

//=============================================================================================================

float SurfaceChecks::minSurfaceDistance(const MNEBemSurface& s1, const MNEBemSurface& s2)
{
    float minDist = 1.0f;
    for (int k1 = 0; k1 < s1.np; ++k1) {
        for (int k2 = 0; k2 < s2.np; ++k2) {
            float dist = (s1.rr.row(k1) - s2.rr.row(k2)).norm();
            if (dist < minDist) {
                minDist = dist;
            }
        }
    }
    return minDist;
}

//=============================================================================================================

bool SurfaceChecks::checkThicknesses(const QVector<MNEBemSurface>& surfs)
{
    for (int k = 0; k < surfs.size() - 1; ++k) {
        fprintf(stderr, "Checking distance between %s and %s surfaces...\n",
                qPrintable(getNameOf(surfs[k].id)),
                qPrintable(getNameOf(surfs[k + 1].id)));
        float dist = minSurfaceDistance(surfs[k], surfs[k + 1]);
        fprintf(stderr, "Minimum distance between the %s and %s surfaces is approximately %6.1f mm\n",
                qPrintable(getNameOf(surfs[k].id)),
                qPrintable(getNameOf(surfs[k + 1].id)),
                1000.0f * dist);
    }
    return true;
}

//=============================================================================================================

bool SurfaceChecks::checkSurfaceSize(const MNEBemSurface& surf)
{
    const float MINSIZE = 0.05f;

    Vector3f minDim = Vector3f::Zero();
    Vector3f maxDim = Vector3f::Zero();

    for (int k = 0; k < surf.np; ++k) {
        for (int c = 0; c < 3; ++c) {
            if (surf.rr(k, c) < minDim(c))
                minDim(c) = surf.rr(k, c);
            if (surf.rr(k, c) > maxDim(c))
                maxDim(c) = surf.rr(k, c);
        }
    }

    for (int c = 0; c < 3; ++c) {
        if (maxDim(c) - minDim(c) < MINSIZE) {
            qCritical() << "Dimensions of the surface" << getNameOf(surf.id)
                         << "seem too small (" << 1000.0f * (maxDim(c) - minDim(c)) << "mm)."
                         << "Maybe the unit of measure is meters instead of mm.";
            return false;
        }
    }
    return true;
}

//=============================================================================================================

void SurfaceChecks::reportTriangleAreas(const MNEBemSurface& surf, const QString& name)
{
    if (surf.tri_area.size() == 0) {
        fprintf(stderr, "No triangle area data available for %s\n", qPrintable(name));
        return;
    }

    double minArea = surf.tri_area.minCoeff();
    double maxArea = surf.tri_area.maxCoeff();

    fprintf(stderr, "Triangle areas on %s surface: %8.3f ... %8.3f mm^2\n",
            qPrintable(name),
            1e6 * minArea,
            1e6 * maxArea);
}
