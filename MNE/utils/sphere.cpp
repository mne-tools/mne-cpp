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
