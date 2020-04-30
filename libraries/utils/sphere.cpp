//=============================================================================================================
/**
 * @file     sphere.cpp
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
 *
 * @brief    Definition of the Sphere Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sphere.h"
#include "simplex_algorithm.h"

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

#include <iostream>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Sphere::Sphere(const Vector3f& center, float radius)
: m_center(center)
, m_r(radius)
{
}

//=============================================================================================================

Sphere Sphere::fit_sphere(const MatrixX3f& points)
{
    const VectorXf& x = points.col(0);
    const VectorXf& y = points.col(1);
    const VectorXf& z = points.col(2);

    VectorXf point_means = points.colwise().mean();

    VectorXf x_mean_free = x.array() - point_means(0);
    VectorXf y_mean_free = y.array() - point_means(1);
    VectorXf z_mean_free = z.array() - point_means(2);

    Matrix3f A;
    A << (x.cwiseProduct(x_mean_free)).mean(), 2*(x.cwiseProduct(y_mean_free)).mean(), 2*(x.cwiseProduct(z_mean_free)).mean(),
                                            0,   (y.cwiseProduct(y_mean_free)).mean(), 2*(y.cwiseProduct(z_mean_free)).mean(),
                                            0,                                      0,   (z.cwiseProduct(z_mean_free)).mean();

    Matrix3f A_T = A.transpose();
    A += A_T;

    Vector3f b;
    VectorXf sq_sum = x.array().pow(2)+y.array().pow(2)+z.array().pow(2);
    b << (sq_sum.cwiseProduct(x_mean_free)).mean(),
         (sq_sum.cwiseProduct(y_mean_free)).mean(),
         (sq_sum.cwiseProduct(z_mean_free)).mean();

    Vector3f center = A.ldlt().solve(b);

    MatrixX3f tmp(points.rows(),3);
    tmp.col(0) = x.array() - center(0);
    tmp.col(1) = y.array() - center(1);
    tmp.col(2) = z.array() - center(2);

    float r = sqrt(tmp.array().pow(2).rowwise().sum().mean());

    return Sphere(center, r);
}

//=============================================================================================================

Sphere Sphere::fit_sphere_simplex(const MatrixX3f& points, double simplex_size)
{
    VectorXf center;
    float R;
    if(fit_sphere_to_points( points, simplex_size, center, R)) {
        return Sphere(center, R);
    }

    return Sphere(Vector3f(), 0.0f);
}

//=============================================================================================================

bool Sphere::fit_sphere_to_points(float **rr, int np, float simplex_size, float *r0, float *R)
{
    MatrixXf rr_eigen(np,3);
    VectorXf r0_eigen(3);

    MatrixX3f dig_rr_eigen;
    for (int k = 0; k < np; k++) {
        rr_eigen(k, 0) = rr[k][0];
        rr_eigen(k, 1) = rr[k][1];
        rr_eigen(k, 2) = rr[k][2];
    }

    if(rr_eigen.rows() < 0) {
        std::cout << "Sphere::fit_sphere_to_points - No points were passed." << std::endl;
        return false;
    }

    r0_eigen(0) = r0[0];
    r0_eigen(1) = r0[1];
    r0_eigen(2) = r0[2];

    bool state = UTILSLIB::Sphere::fit_sphere_to_points(rr_eigen,simplex_size,r0_eigen,*R);

    r0[0] = r0_eigen(0);
    r0[1] = r0_eigen(1);
    r0[2] = r0_eigen(2);

    return state;
}

//=============================================================================================================

bool Sphere::fit_sphere_to_points(const MatrixXf &rr, float simplex_size, VectorXf &r0, float &R)
{
//    int   np = rr.rows();

    /*
     * Find the optimal sphere origin
     */
    fitUserRecNew user;
    float      ftol            = 1e-5f;
    int        max_eval        = 500;
    int        report_interval = -1;
    int        neval;
    MatrixXf   init_simplex;
    VectorXf   init_vals(4);

    VectorXf   cm(3);
    float      R0 = 0.1f;

    user.rr = rr;

    calculate_cm_ave_dist(rr, cm, R0);// [done]

    init_simplex = make_initial_simplex( cm, simplex_size );

    user.report = false;

    for (int k = 0; k < 4; k++) {
        init_vals[k] = fit_eval( static_cast<VectorXf>(init_simplex.row(k)), &user );
    }

    user.report = false;

    //Start the minimization
    if(!SimplexAlgorithm::simplex_minimize<float>(  init_simplex,   /* The initial simplex */
                                                    init_vals,      /* Function values at the vertices */
                                                    ftol,           /* Relative convergence tolerance */
                                                    fit_eval,       /* The function to be evaluated */
                                                    &user,          /* Data to be passed to the above function in each evaluation */
                                                    max_eval,       /* Maximum number of function evaluations */
                                                    neval,          /* Number of function evaluations */
                                                    report_interval,/* How often to report (-1 = no_reporting) */
                                                    report_func))   /* The function to be called when reporting */
    {
        return false;
    }

    r0 = init_simplex.row(0);
    R = opt_rad(r0, &user);

    return true;
}

//=============================================================================================================

bool Sphere::report_func(int loop, const VectorXf &fitpar, double fval)
{
    /*
     * Report periodically
     */
    const VectorXf& r0 = fitpar;

    std::cout << "loop: " << loop << "; r0: " << 1000*r0[0] << ", r1: " << 1000*r0[1] << ", r2: " << 1000*r0[2] << "; fval: " << fval << std::endl;

    return true;
}

//=============================================================================================================

void Sphere::calculate_cm_ave_dist(const MatrixXf &rr, VectorXf &cm, float &avep)
{
    cm = rr.colwise().mean();
    MatrixXf diff = rr.rowwise() - cm.transpose();
    avep = diff.rowwise().norm().mean();
}

//=============================================================================================================

MatrixXf Sphere::make_initial_simplex(const VectorXf &pars, float size)
{
    /*
     * Make the initial tetrahedron
     */
    int npar = pars.size();

    MatrixXf simplex = MatrixXf::Zero(npar+1,npar);

    simplex.rowwise() += pars.transpose();

    for (int k = 1; k < npar+1; k++) {
        simplex(k,k-1) += size;
    }

    return simplex;
}

//=============================================================================================================

float Sphere::fit_eval(const VectorXf &fitpar, const void  *user_data)
{
    /*
     * Calculate the cost function value
     * Optimize for the radius inside here
     */
    const fitUserNew& user = (fitUserNew)user_data;
    const VectorXf& r0 = fitpar;

    float F;

    MatrixXf diff = user->rr.rowwise() - r0.transpose();
    VectorXf one = diff.rowwise().norm();

    float sum = one.sum();
    float sum2 = one.dot(one);

    F = sum2 - sum*sum/user->rr.rows();

    if(user->report)
        std::cout << "r0: " << 1000*r0[0] << ", r1: " << 1000*r0[1] << ", r2: " << 1000*r0[2] << "; R: " << 1000*sum/user->rr.rows() << "; fval: "<<F<<std::endl;

    return F;
}

//=============================================================================================================

float Sphere::opt_rad(const VectorXf &r0,const fitUserNew user)
{
  MatrixXf diff = user->rr.rowwise() - r0.transpose();
  return diff.rowwise().norm().mean();
}
