//=============================================================================================================
/**
 * @file     mne_project_to_surface.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen. All rights reserved.
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
 * @brief     MNEProjectToSurface class declaration.
 *
 */

#ifndef MNELIB_MNEPROJECTTOSURFACE_H
#define MNELIB_MNEPROJECTTOSURFACE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB {

//=============================================================================================================
// MNELIB FORWARD DECLARATIONS
//=============================================================================================================

class MNEBemSurface;
class MNESurface;

//TODO this needs to be removed - this has to be a function not a class!!!

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */

class MNESHARED_EXPORT MNEProjectToSurface
{

public:
    typedef QSharedPointer<MNEProjectToSurface> SPtr;            /**< Shared pointer type for MNEProjectToSurface. */
    typedef QSharedPointer<const MNEProjectToSurface> ConstSPtr; /**< Const shared pointer type for MNEProjectToSurface. */

    //=========================================================================================================
    /**
     * Constructs a MNEProjectToSurface object.
     */
    MNEProjectToSurface();

    //=========================================================================================================
    /**
     * Constructs a MNEProjectToSurface with the data of a MNEBemSurface object.
     * @brief MNEProjectToSurface
     * @param[in] p_MNEBemSurf   The MNEBemSurface to which is to be projected.
     */
    MNEProjectToSurface(const MNELIB::MNEBemSurface &p_MNEBemSurf);

    //=========================================================================================================
    /**
     * Constructs a MNEProjectToSurface with the data of a MNESurf object.
     * @brief MNEProjectToSurface
     * @param[in] p_MNESurf      The MNESurface to which is to be projected.
     */
    MNEProjectToSurface(const MNELIB::MNESurface &p_MNESurf);

    //=========================================================================================================
    /**
     * Projects a set of points r on the Surface
     *
     * @brief mne_find_closest_on_surface
     *
     * @param[in] r         Set of pionts, which are to be projectied.
     * @param[in] np        number of points.
     * @param[out] rTri     set of points on the surface.
     * @param[out] nearest  Triangle of the new point.
     * @param[out] dist     Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool mne_find_closest_on_surface(const Eigen::MatrixXf &r, const int np, Eigen::MatrixXf &rTri,
                                     Eigen::VectorXi &nearest, Eigen::VectorXf &dist);

protected:

private:
    //=========================================================================================================
    /**
     * Projects a point r on the Surface
     *
     * @brief mne_project_to_surface
     *
     * @param[in] r         Piont, which is to be projectied.
     * @param[out] rTri     Point on the surface.
     * @param[out] bestTri  Triangle of the new point.
     * @param[out] bestDist Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool mne_project_to_surface(const Eigen::Vector3f &r, Eigen::Vector3f &rTri, int &bestTri, float &bestDist);

    //=========================================================================================================
    /**
     * Finds the nearest point to a point r on a given triangle.
     *
     * @brief nearest_triangle_point
     *
     * @param[in] r     Point in space.
     * @param[in] tri   The Triangle of the surface.
     * @param[out] p    Coordiante in Triangel System (rTri = r1 + p*r12 +q*r13).
     * @param[out] q    Coordiante in Triangel System (rTri = r1 + p*r12 +q*r13).
     * @param[out] dist Distance between r and rTri.
     *
     * @return true if succeeded, false otherwise.
     */
    bool nearest_triangle_point(const Eigen::Vector3f &r, const int tri, float &p, float &q, float &dist);

    //=========================================================================================================
    /**
     * Converts a point given in triangel coordinates (p,q) to the kartesian system.
     *
     * @brief project_to_triangle
     *
     * @param[out] rTri Coordiante in kartesian System.
     * @param[in] p     Coordiante in Triangel System (r = r1 + p*r12 +q*r13).
     * @param[in] q     Coordiante in Triangel System (r = r1 + p*r12 +q*r13).
     * @param[in] tri   The Triangle of the surface.
     *
     * @return true if succeeded, false otherwise.
     */
    bool project_to_triangle(Eigen::Vector3f &rTri, const float p, const float q, const int tri);

    Eigen::MatrixX3f r1;         /**< Cartesian Vector to the first triangel corner. */
    Eigen::MatrixX3f r12;        /**< Cartesian Vector from the first to the second triangel corner. */
    Eigen::MatrixX3f r13;        /**< Cartesian Vector from the first to the third triangel corner. */
    Eigen::MatrixX3f nn;         /**< Cartesian Vector of the triangle plane normal. */
    Eigen::VectorXf a;           /**< r12*r12. */
    Eigen::VectorXf b;           /**< r13*r13. */
    Eigen::VectorXf c;           /**< r12*r13. */
    Eigen::VectorXf det;         /**< Determinant of the Matrix [a c, c b]. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace MNELIB

#endif // MNELIB_MNEPROJECTTOSURFACE_H
