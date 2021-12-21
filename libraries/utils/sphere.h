//=============================================================================================================
/**
 * @file     sphere.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     April, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Christoph Dinh. All rights reserved.
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
 * @brief    Sphere class declaration.
 *
 */

#ifndef SPHERE_H
#define SPHERE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// TYPEDEFS
//=============================================================================================================

typedef struct {
    Eigen::MatrixXf rr;
     bool   report;
} *fitUserNew,fitUserRecNew;

//=============================================================================================================
/**
 * Sphere descritpion
 *
 * @brief Describes a 3D sphere object
 */
class UTILSSHARED_EXPORT Sphere
{
public:

    //=========================================================================================================
    /**
     * Constructs the Sphere
     *
     * @param[in] center     The sphere's center.
     * @param[in] radius     The sphere's radius.
     */
    Sphere(const Eigen::Vector3f& center, float radius);

    //=========================================================================================================
    /**
     * Fits a sphere to a point cloud. Algorithm by Alan Jennings, University of Dayton
     *
     * @param[in] points     n x 3 matrix of cartesian data to fit the sphere to.
     *
     * @return the fitted sphere.
     */
    static Sphere fit_sphere(const Eigen::MatrixX3f& points);

    //=========================================================================================================
    /**
     * Fits a sphere to a point cloud.
     *
     * @param[in] points         n x 3 matrix of cartesian data to fit the sphere to.
     * @param[in] simplex_size   The simplex size.
     *
     * @return the fitted sphere.
     */
    static Sphere fit_sphere_simplex(const Eigen::MatrixX3f& points, double simplex_size = 2e-2);

    //=========================================================================================================
    /**
     * The radius of the sphere.
     *
     * @return the fitted sphere.
     */
    Eigen::Vector3f& center() { return m_center; }

    //=========================================================================================================
    /**
     * The radius of the sphere.
     *
     * @return the fitted sphere.
     */
    float& radius() { return m_r; }

    //=========================================================================================================
    /**
     * Fits a sphere to a point cloud.
     *
     * @param[in] rr             n x 3 matrix of cartesian data to fit the sphere to.
     * @param[in] simplex_size   The simplex size.
     * @param[out] r0            center (1 x 3 matrix) of the sphere.
     * @param[out] R             Radius.
     *
     * @return true if successful.
     */
    static bool fit_sphere_to_points(const Eigen::MatrixXf &rr, float simplex_size, Eigen::VectorXf &r0, float &R);
    static bool fit_sphere_to_points(float **rr, int np, float simplex_size, float *r0, float *R);

private:
    Eigen::Vector3f m_center;   /**< Sphere's center. */
    float m_r;                  /**< Sphere's radius. */

    //=========================================================================================================
    /**
     * Calculates the average
     *
     * @param[in] rr     n x 3 matrix of cartesian data to fit the sphere to.
     * @param[out] cm    The average center of the caretsian data (1 x 3 matrix).
     * @param[out] avep  The average distance to the average center.
     */
    static void calculate_cm_ave_dist(const Eigen::MatrixXf &rr, Eigen::VectorXf &cm, float &avep);

    //=========================================================================================================
    /**
     * Creates the initial simplex.
     *
     * @param[in] pars   The simplex center (1 x 3 matrix).
     * @param[in] size   The simplex size.
     *
     * @return the inital simplex.
     */
    static Eigen::MatrixXf make_initial_simplex(const Eigen::VectorXf &pars, float size);

    //=========================================================================================================
    /**
     * The simplex cost function
     *
     * @param[in] fitpar     A simplex vertex to evaluate.
     * @param[in] user_data  The user data containing the n x 3 matrix of cartesian data.
     *
     * @return the distance (cost) of the given vertex (sphere center).
     */
    static float fit_eval(const Eigen::VectorXf &fitpar, const void  *user_data);

    //=========================================================================================================
    /**
     * The report function, called to rpeort the optimization status
     *
     * @param[in] loop       The current iteration loop.
     * @param[in] fitpar     The currently best fitting simplex vertex.
     * @param[in] fval       The optimization value.
     *
     * @return true if reporting was successful.
     */
    static bool report_func(int loop, const Eigen::VectorXf &fitpar, double fval);

    //=========================================================================================================
    /**
     * Calculates the optimal radius based on a given center.
     *
     * @param[in] r0      The center.
     * @param[in] user    The user data containing the n x 3 matrix of cartesian data.
     *
     * @return the optimal radius.
     */
    static float opt_rad(const Eigen::VectorXf &r0, const fitUserNew user);
};
} // NAMESPACE

#endif // SPHERE_H
