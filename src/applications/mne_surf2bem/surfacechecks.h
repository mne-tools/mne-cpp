//=============================================================================================================
/**
 * @file     surfacechecks.h
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
 * @brief    SurfaceChecks class declaration.
 *
 *           Ported from surface_checks.c in the original MNE C tool mne_surf2bem
 *           by Matti Hamalainen (SVN $Id: surface_checks.c 3186).
 *
 *           Provides topology verification for BEM surfaces:
 *             - Solid angle test for surface completeness
 *             - Containment test (brain inside skull inside skin)
 *             - Triangle area reporting
 *             - Inter-surface distance measurement
 *             - Surface size validation (meters vs mm detection)
 *
 */

#ifndef SURFACECHECKS_H
#define SURFACECHECKS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <mne/mne_bem_surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>
#include <QString>

//=============================================================================================================
// DEFINE NAMESPACE MNESURF2BEM
//=============================================================================================================

namespace MNESURF2BEM {

//=============================================================================================================
/**
 * Provides topology checks for BEM surfaces.
 *
 * Ported from surface_checks.c in the original MNE C tool by Matti Hamalainen.
 * All checks use the van Oosterom solid angle formula to verify surface
 * completeness and proper nesting.
 *
 * @brief BEM surface topology checks.
 */
class SurfaceChecks
{
public:
    //=========================================================================================================
    /**
     * Returns a human-readable name for a BEM surface id.
     *
     * @param[in] id  BEM surface id (FIFFV_BEM_SURF_ID_*).
     *
     * @return Human-readable name string.
     */
    static QString getNameOf(int id);

    //=========================================================================================================
    /**
     * Checks whether a surface is complete (closed) by computing the sum of
     * solid angles from the center of mass. For a complete surface, this
     * should equal 4*pi (normalized to 1.0).
     *
     * @param[in] surf  The BEM surface to check.
     *
     * @return true if the surface is complete.
     */
    static bool isCompleteSurface(const MNELIB::MNEBemSurface& surf);

    //=========================================================================================================
    /**
     * Checks that all surfaces are complete and properly nested
     * (each surface inside the previous one).
     *
     * When nsurf == 3, expects ordering: head (outermost), skull, brain (innermost).
     *
     * @param[in] surfs  Vector of BEM surfaces to check.
     *
     * @return true if all checks pass.
     */
    static bool checkSurfaces(const QVector<MNELIB::MNEBemSurface>& surfs);

    //=========================================================================================================
    /**
     * Reports the minimum distance between adjacent surfaces.
     *
     * @param[in] surfs  Vector of BEM surfaces.
     *
     * @return true if thickness check passes.
     */
    static bool checkThicknesses(const QVector<MNELIB::MNEBemSurface>& surfs);

    //=========================================================================================================
    /**
     * Checks that the surface coordinate dimensions are reasonable.
     * Detects if coordinates might be in meters instead of the expected
     * millimeters (FIFF convention is meters, but surface files use mm).
     *
     * @param[in] surf  The BEM surface to check.
     *
     * @return true if surface dimensions look reasonable.
     */
    static bool checkSurfaceSize(const MNELIB::MNEBemSurface& surf);

    //=========================================================================================================
    /**
     * Reports the minimum and maximum triangle areas for a surface.
     *
     * @param[in] surf  The BEM surface.
     * @param[in] name  Display name for the surface.
     */
    static void reportTriangleAreas(const MNELIB::MNEBemSurface& surf, const QString& name);

private:
    //=========================================================================================================
    /**
     * Computes the solid angle subtended by a single triangle as seen from
     * a given point, using van Oosterom's formula.
     *
     * @param[in] from   The observation point (3D).
     * @param[in] v0     First triangle vertex.
     * @param[in] v1     Second triangle vertex.
     * @param[in] v2     Third triangle vertex.
     *
     * @return Solid angle in steradians.
     */
    static double solidAngle(const Eigen::Vector3f& from,
                             const Eigen::Vector3f& v0,
                             const Eigen::Vector3f& v1,
                             const Eigen::Vector3f& v2);

    //=========================================================================================================
    /**
     * Computes the total solid angle subtended by all triangles in a surface
     * as seen from a given point.
     *
     * @param[in] from  The observation point (3D).
     * @param[in] surf  The BEM surface.
     *
     * @return Total solid angle in steradians.
     */
    static double sumSolids(const Eigen::Vector3f& from,
                            const MNELIB::MNEBemSurface& surf);

    //=========================================================================================================
    /**
     * Checks whether all vertices of surface 'from' are inside surface 'to'.
     *
     * @param[in] from  The surface that should be inside.
     * @param[in] to    The enclosing surface.
     *
     * @return true if 'from' is completely inside 'to'.
     */
    static bool isInside(const MNELIB::MNEBemSurface& from,
                         const MNELIB::MNEBemSurface& to);

    //=========================================================================================================
    /**
     * Computes the minimum distance between vertices of two surfaces.
     *
     * @param[in] s1  First surface.
     * @param[in] s2  Second surface.
     *
     * @return Minimum distance in meters.
     */
    static float minSurfaceDistance(const MNELIB::MNEBemSurface& s1,
                                   const MNELIB::MNEBemSurface& s2);
};

} // namespace MNESURF2BEM

#endif // SURFACECHECKS_H
