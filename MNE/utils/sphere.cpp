//=============================================================================================================
/**
* @file     sphere.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the MNEMath Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sphere.h"
#include "minimizersimplex.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Dense>


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <iostream>
#include <algorithm>    // std::sort
#include <vector>       // std::vector

//DEBUG fstream
//#include <fstream>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#ifndef OK
#define OK 0
#endif

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Sphere::Sphere( Vector3d center, double radius )
: m_center(center)
, m_r(radius)
{
}


//*************************************************************************************************************

//    function [Center,Radius] = sphereFit(X)
//    % this fits a sphere to a collection of data using a closed form for the
//    % solution (opposed to using an array the size of the data set).
//    % Minimizes Sum((x-xc)^2+(y-yc)^2+(z-zc)^2-r^2)^2
//    % x,y,z are the data, xc,yc,zc are the sphere's center, and r is the radius

//    % Assumes that points are not in a singular configuration, real numbers, ...
//    % if you have coplanar data, use a circle fit with svd for determining the
//    % plane, recommended Circle Fit (Pratt method), by Nikolai Chernov
//    % http://www.mathworks.com/matlabcentral/fileexchange/22643

//    % Input:
//    % X: n x 3 matrix of cartesian data
//    % Outputs:
//    % Center: Center of sphere
//    % Radius: Radius of sphere
//    % Author:
//    % Alan Jennings, University of Dayton

//    A=[mean(X(:,1).*(X(:,1)-mean(X(:,1)))), ...
//        2*mean(X(:,1).*(X(:,2)-mean(X(:,2)))), ...
//        2*mean(X(:,1).*(X(:,3)-mean(X(:,3)))); ...
//        0, ...
//        mean(X(:,2).*(X(:,2)-mean(X(:,2)))), ...
//        2*mean(X(:,2).*(X(:,3)-mean(X(:,3)))); ...
//        0, ...
//        0, ...
//        mean(X(:,3).*(X(:,3)-mean(X(:,3))))];
//    A=A+A.';
//    B=[mean((X(:,1).^2+X(:,2).^2+X(:,3).^2).*(X(:,1)-mean(X(:,1))));...
//        mean((X(:,1).^2+X(:,2).^2+X(:,3).^2).*(X(:,2)-mean(X(:,2))));...
//        mean((X(:,1).^2+X(:,2).^2+X(:,3).^2).*(X(:,3)-mean(X(:,3))))];
//    Center=(A\B).';
//    Radius=sqrt(mean(sum([X(:,1)-Center(1),X(:,2)-Center(2),X(:,3)-Center(3)].^2,2)));

Sphere Sphere::fit_sphere(const MatrixX3d& points)
{
    const VectorXd& x = points.col(0);
    const VectorXd& y = points.col(1);
    const VectorXd& z = points.col(2);

    VectorXd point_means = points.colwise().mean();

    VectorXd x_mean_free = x.array() - point_means(0);
    VectorXd y_mean_free = y.array() - point_means(1);
    VectorXd z_mean_free = z.array() - point_means(2);

    Matrix3d A;
    A << (x.cwiseProduct(x_mean_free)).mean(), 2*(x.cwiseProduct(y_mean_free)).mean(), 2*(x.cwiseProduct(z_mean_free)).mean(),
                                            0,   (y.cwiseProduct(y_mean_free)).mean(), 2*(y.cwiseProduct(z_mean_free)).mean(),
                                            0,                                      0,   (z.cwiseProduct(z_mean_free)).mean();

    Matrix3d A_T = A.transpose();
    A += A_T;

    Vector3d b;
    VectorXd sq_sum = x.array().pow(2)+y.array().pow(2)+z.array().pow(2);
    b << (sq_sum.cwiseProduct(x_mean_free)).mean(),
         (sq_sum.cwiseProduct(y_mean_free)).mean(),
         (sq_sum.cwiseProduct(z_mean_free)).mean();

    Vector3d center = A.ldlt().solve(b);

    MatrixX3d tmp(points.rows(),3);
    tmp.col(0) = x.array() - center(0);
    tmp.col(1) = y.array() - center(1);
    tmp.col(2) = z.array() - center(2);

    double r = sqrt(tmp.array().pow(2).rowwise().sum().mean());

    return Sphere(center, r);
}


//*************************************************************************************************************

Sphere Sphere::fit_sphere_simplex(const MatrixX3f& points, double simplex_size)
{
    VectorXf r0;
    float R;
    fit_sphere_to_points_new( points, simplex_size, r0, R);

    Vector3d center;

    center[0] = r0[0];
    center[1] = r0[1];
    center[2] = r0[2];

    return Sphere(center, R);
}


//*************************************************************************************************************
//ToDo Replace LayoutMaker::fit_sphere_to_points
bool Sphere::fit_sphere_to_points_new ( const MatrixXf &rr,
                                        float simplex_size,
                                        VectorXf &r0,
                                        float &R )
{
//    int   np = rr.rows();

    /*
    * Find the optimal sphere origin
    */
    fitUserRecNew user;
    float      ftol            = (float) 1e-5;
    int        max_eval        = 500;
    int        report_interval = -1;
    int        neval;
    MatrixXf   init_simplex;
    VectorXf   init_vals(4);

    VectorXf   cm(3);
    float      R0;
    int        k;

    bool        res = false;

    user.rr = rr;

    R0 = (float) 0.1;
    calculate_cm_ave_dist_new(rr, cm, R0);// [done]

    init_simplex = make_initial_simplex_new( cm, simplex_size );

    user.report = true;

    for (k = 0; k < 4; k++) {
        init_vals[k] = fit_eval_new( static_cast<VectorXf>(init_simplex.row(k)), 3, &user );
        qDebug() << "k" << k << "; init_vals" << init_vals[k];
    }

    user.report = false;

    //Start the minimization
    if(MinimizerSimplex::mne_simplex_minimize(init_simplex, /* The initial simplex */
                            init_vals,                      /* Function values at the vertices */
                            3,                              /* Number of variables */
                            ftol,                           /* Relative convergence tolerance */
                            fit_eval_new,                   /* The function to be evaluated */
                            &user,                          /* Data to be passed to the above function in each evaluation */
                            max_eval,                       /* Maximum number of function evaluations */
                            neval,                          /* Number of function evaluations */
                            report_interval,                /* How often to report (-1 = no_reporting) */
                            report_func_new) != OK)             /* The function to be called when reporting */
        return false;

    r0[0] = init_simplex(0,0);
    r0[1] = init_simplex(0,1);
    r0[2] = init_simplex(0,2);
    R = opt_rad_new(r0,&user);

    res = true;

    return res;
}


//*************************************************************************************************************
//ToDo Replace LayoutMaker::report_func
int Sphere::report_func_new(int loop,
                             const VectorXf &fitpar,
                             int npar,
                             double fval)
{
    Q_UNUSED(npar);

    /*
    * Report periodically
    */
    VectorXf r0 = fitpar;

    std::cout << "loop: " << loop << "; r0: " << 1000*r0[0] << ", r1: " << 1000*r0[1] << ", r2: " << 1000*r0[2] << "; fval: " << fval << std::endl;

    return OK;
}

//*************************************************************************************************************
//ToDo Replace LayoutMaker::calculate_cm_ave_dist

void Sphere::calculate_cm_ave_dist_new (const MatrixXf &rr,
                                        VectorXf &cm,
                                        float &avep)
{
    cm = rr.colwise().mean();
    MatrixXf diff = rr.rowwise() - cm.transpose();
    avep = diff.rowwise().norm().mean();
}


//*************************************************************************************************************
//ToDo Replace LayoutMaker::make_initial_simplex
MatrixXf Sphere::make_initial_simplex_new(  const VectorXf &pars,
                                            float size )
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


//*************************************************************************************************************
//ToDo Replace LayoutMaker::fit_eval
float Sphere::fit_eval_new (    const VectorXf &fitpar,
                                int   npar,
                                void  *user_data)
{
    Q_UNUSED(npar)

    /*
    * Calculate the cost function value
    * Optimize for the radius inside here
    */
    fitUserNew user = (fitUserNew)user_data;
    const VectorXf& r0 = fitpar;

    float F;

    MatrixXf diff = (user->rr.rowwise() - r0.transpose())*-1;
    VectorXf one = diff.rowwise().norm();

    float sum = one.sum();
    float sum2 = one.dot(one);

    F = sum2 - sum*sum/user->rr.rows();

    if(user->report)
        std::cout << "r0: " << 1000*r0[0] << ", r1: " << 1000*r0[1] << ", r2: " << 1000*r0[2] << "; R: " << 1000*sum/user->rr.rows() << "; fval: "<<F<<std::endl;

    return F;
}


//*************************************************************************************************************
//ToDo Replace LayoutMaker::opt_rad
float Sphere::opt_rad_new(VectorXf &r0,fitUserNew user)
{
  float sum, one;
  VectorXf diff(3);
  int   k;

  for (k = 0, sum = 0.0; k < user->rr.rows(); k++) {
    diff = r0 - static_cast<VectorXf>(user->rr.row(k));
    one = sqrt(pow(diff(0),2) + pow(diff(1),2) + pow(diff(2),2));
    sum  += one;
  }

  return sum/user->rr.rows();
}
