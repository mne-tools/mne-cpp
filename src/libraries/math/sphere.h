//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     sphere.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Best-fit sphere from a 3-D point cloud with closed-form and Nelder–Mead solvers.
 *
 * @ref UTILSLIB::Sphere fits a single sphere
 * @f$(\mathbf{c}, r)@f$ to an @c n×3 point cloud and is the geometric
 * primitive used to build the spherical head models consumed by
 * FWDLIB's BEM solvers and by INVERSELIB's depth weighting. Two solvers
 * are provided: a closed-form algebraic fit due to Alan Jennings
 * (University of Dayton) that solves a single linear system in
 * @c O(n) and returns the algebraic best fit in one shot, and a
 * Nelder–Mead refinement (@ref fit_sphere_simplex) that minimises the
 * geometric residual @f$\sum_i (\|\mathbf{p}_i - \mathbf{c}\| - r)^2@f$
 * via @ref UTILSLIB::SimplexAlgorithm and is more robust on noisy
 * digitiser data where the algebraic fit is biased toward outliers.
 *
 * The class itself is a lightweight value type carrying @c (center, radius);
 * the fit routines are static factories so the same object layout is
 * shared between MRI head-shape extraction, HPI coil fitting and the
 * spherical-shell BEM construction in FWDLIB.
 */

#ifndef SPHERE_H
#define SPHERE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

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

/** @brief Cost-function workspace for @ref Sphere::fit_sphere_simplex: holds the @c nx3 point cloud and a verbose-report flag. */
struct FitUser {
    Eigen::MatrixXf rr;
    bool report;
};

//=============================================================================================================
/**
 * Best-fit sphere primitive carrying centre and radius, with static
 * factories for the closed-form algebraic fit (Jennings) and the
 * Nelder–Mead geometric-residual refinement. Used to build spherical
 * head models, fit HPI coils and digitiser scalps.
 *
 * @brief 3-D sphere value type with algebraic and Nelder–Mead best-fit factories.
 */
class MATHSHARED_EXPORT Sphere
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
     * The center of the sphere.
     *
     * @return reference to the sphere's center.
     */
    Eigen::Vector3f& center() { return m_center; }

    //=========================================================================================================
    /**
     * The radius of the sphere.
     *
     * @return reference to the sphere's radius.
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
    static bool report_func(int loop, const Eigen::VectorXf &fitpar, double fval_lo, double fval_hi, double par_diff);

    //=========================================================================================================
    /**
     * Calculates the optimal radius based on a given center.
     *
     * @param[in] r0      The center.
     * @param[in] user    The user data containing the n x 3 matrix of cartesian data.
     *
     * @return the optimal radius.
     */
    static float opt_rad(const Eigen::VectorXf &r0, const FitUser* user);
};
} // NAMESPACE

#endif // SPHERE_H
