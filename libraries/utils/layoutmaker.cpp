//=============================================================================================================
/**
 * @file     layoutmaker.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.0
 * @date     September, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the LayoutMaker class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutmaker.h"
#include "simplex_algorithm.h"
#include "sphere.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QTextStream>
#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINES
//=============================================================================================================

#ifndef EPS
#define EPS 1e-6
#endif

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

bool LayoutMaker::makeLayout(const QList<QVector<float> > &inputPoints,
                             QList<QVector<float> > &outputPoints,
                             const QStringList &names,
                             QFile &outFile,
                             bool do_fit,
                             float prad,
                             float w,
                             float h,
                             bool writeFile,
                             bool mirrorXAxis,
                             bool mirrorYAxis)
{
    /*
     * Automatically make a layout according to the
     * channel locations in inputPoints
     */
    VectorXf    r0(3);
    VectorXf    rr(3);
    float       rad,th,phi;

    float       xmin,xmax,ymin,ymax;
    int         k;
    int         nchan = inputPoints.size();

    MatrixXf rrs(nchan,3);
    VectorXf xx(nchan);
    VectorXf yy(nchan);

    if (nchan <= 0) {
        std::cout << "No input points to lay out.\n";
        return false;
    }

    //Fill matrix with 3D points
    for(k = 0; k<nchan; k++) {
        rrs(k,0) = inputPoints.at(k)[0]; //x
        rrs(k,1) = inputPoints.at(k)[1]; //y
        rrs(k,2) = inputPoints.at(k)[2]; //z
    }

    std::cout << "Channels found for layout: " << nchan << "\n";

    //Fit to sphere if wanted by the user
    if (!do_fit) {
        std::cout << "Using default origin:" << r0[0] << ", " << r0[1] << ", " << r0[2] << "\n";
    }
    else {
        Sphere sphere = Sphere::fit_sphere_simplex(rrs, 0.05);

        r0 = sphere.center();
        rad = sphere.radius();

        std::cout << "best fitting sphere:\n";
        std::cout << "torigin: " << r0[0] << ", " << r0[1] << ", " << r0[2] << std::endl << "; tradius: " << rad << "\n";
    }

    /*
     * Do the azimuthal equidistant projection
     */
    for (k = 0; k < nchan; k++) {
        rr = r0 - static_cast<VectorXf>(rrs.row(k));
        sphere_coord(rr[0],rr[1],rr[2],&rad,&th,&phi);
        xx[k] = prad*(2.0*th/M_PI)*cos(phi);
        yy[k] = prad*(2.0*th/M_PI)*sin(phi);
    }

    /*
     * Find suitable range of viewports
     */
    xmin = xmax = xx[0];
    ymin = ymax = yy[0];

    for(k = 1; k < nchan; k++) {
        if (xx[k] > xmax)
            xmax = xx[k];
        else if (xx[k] < xmin)
            xmin = xx[k];
        if (yy[k] > ymax)
            ymax = yy[k];
        else if (yy[k] < ymin)
            ymin = yy[k];
    }

    if(xmin == xmax || ymin == ymax) {
        std::cout<<"Cannot make a layout. All positions are identical\n";
        return false;
    }

    xmax = xmax + 0.6*w;
    xmin = xmin - 0.6*w;

    ymax = ymax + 0.6*h;
    ymin = ymin - 0.6*h;

    /*
     * Compose the viewports
     */
    QVector<float> point;
    QTextStream out;

    if(writeFile) {
        if (!outFile.open(QIODevice::WriteOnly)) {
            std::cout << "Could not open output file!\n";
            return false;
        }

        out.setDevice(&outFile);
    }

    out << "0.000000 0.000000 0.000000 0.000000" << endl;

    for(k = 0; k < nchan; k++) {
        point.clear();

        if(mirrorXAxis)
            point.append(-(xx[k]-0.5*w));
        else
            point.append(xx[k]-0.5*w);

        if(mirrorYAxis)
            point.append(-(yy[k]-0.5*h));
        else
            point.append(yy[k]-0.5*h);

        outputPoints.append(point);

        if(writeFile) {
            if(k < names.size()) {
                out << k+1 << " " << point[0] << " " << point[1] << " " << w << " " << h << " " << names.at(k) << endl;
            } else {
                out << k+1 << " " << point[0] << " " << point[1] << " " << w << " " << h <<endl;
            }
        }
    }

    if(writeFile) {
        std::cout << "Success while wrtiting to output file.\n";

        outFile.close();
    }

    return true;
}

//=============================================================================================================

bool LayoutMaker::makeLayout(const std::vector<std::vector<float> > &inputPoints,
                             std::vector<std::vector<float> > &outputPoints,
                             const std::vector<std::string> &names,
                             const std::string& outFilePath,
                             bool do_fit,
                             float prad,
                             float w,
                             float h,
                             bool writeFile,
                             bool mirrorXAxis,
                             bool mirrorYAxis)
{
    /*
     * Automatically make a layout according to the
     * channel locations in inputPoints
     */
    VectorXf    r0(3);
    VectorXf    rr(3);
    float       rad,th,phi;

    float       xmin,xmax,ymin,ymax;
    int         nchan = inputPoints.size();

    MatrixXf rrs(nchan,3);
    VectorXf xx(nchan);
    VectorXf yy(nchan);

    if (nchan <= 0) {
        std::cout << "No input points to lay out.\n";
        return false;
    }

    //Fill matrix with 3D points
    for(int k = 0; k < nchan; k++) {
        rrs(k,0) = inputPoints.at(k)[0]; //x
        rrs(k,1) = inputPoints.at(k)[1]; //y
        rrs(k,2) = inputPoints.at(k)[2]; //z
    }

    std::cout << "Channels found for layout: " << nchan << "\n";

    //Fit to sphere if wanted by the user
    if (!do_fit) {
        std::cout << "Using default origin:" << r0[0] << ", " << r0[1] << ", " << r0[2] << "\n";
    }
    else {
        Sphere sphere = Sphere::fit_sphere_simplex(rrs, 0.05);

        r0 = sphere.center();
        rad = sphere.radius();

        std::cout << "best fitting sphere:\n";
        std::cout << "torigin: " << r0[0] << ", " << r0[1] << ", " << r0[2] << std::endl << "; tradius: " << rad << "\n";
    }

    /*
     * Do the azimuthal equidistant projection
     */
    for (int k = 0; k < nchan; k++) {
        rr = r0 - static_cast<VectorXf>(rrs.row(k));
        sphere_coord(rr[0],rr[1],rr[2],&rad,&th,&phi);
        xx[k] = prad*(2.0*th/M_PI)*cos(phi);
        yy[k] = prad*(2.0*th/M_PI)*sin(phi);
    }

    /*
     * Find suitable range of viewports
     */
    xmin = xmax = xx[0];
    ymin = ymax = yy[0];

    for(int k = 1; k < nchan; k++) {
        if (xx[k] > xmax)
            xmax = xx[k];
        else if (xx[k] < xmin)
            xmin = xx[k];
        if (yy[k] > ymax)
            ymax = yy[k];
        else if (yy[k] < ymin)
            ymin = yy[k];
    }

    if(xmin == xmax || ymin == ymax) {
        std::cout<<"Cannot make a layout. All positions are identical\n";
        return false;
    }

    xmax = xmax + 0.6*w;
    xmin = xmin - 0.6*w;

    ymax = ymax + 0.6*h;
    ymin = ymin - 0.6*h;

    /*
     * Compose the viewports
     */
    std::vector<float> point;
    std::ofstream outFile;

    if(writeFile) {
        outFile.open(outFilePath);
        if (outFile.is_open()) {
            std::cout << "Could not open output file!\n";
            return false;
        }
        outFile << "0.000000 0.000000 0.000000 0.000000" << std::endl;
    }


    for(int k = 0; k < nchan; k++) {
        point.clear();

        if(mirrorXAxis)
            point.push_back(-(xx[k]-0.5*w));
        else
            point.push_back(xx[k]-0.5*w);

        if(mirrorYAxis)
            point.push_back(-(yy[k]-0.5*h));
        else
            point.push_back(yy[k]-0.5*h);

        outputPoints.push_back(point);

        if(writeFile) {
            if((k) < names.size()) {
                outFile << k+1 << " " << point[0] << " " << point[1] << " " << w << " " << h << " " << names.at(k) << std::endl;
            } else {
                outFile << k+1 << " " << point[0] << " " << point[1] << " " << w << " " << h << std::endl;
            }
        }
    }

    if(writeFile) {
        std::cout << "Success while wrtiting to output file.\n";
    }

    return true;
}

//=============================================================================================================

void LayoutMaker::sphere_coord (float x,
                              float y,
                              float z,
                              float *r,
                              float *theta,
                              float *phi)
{
  /* Rectangular to spherical coordinates */
  float rxy = sqrt(x*x+y*y);
  if (rxy < EPS) {		/* Let's hope this is reasonable */
     *r = z;
     *theta = 0.0;
     *phi   = 0.0;
  }
  else {
     *r = sqrt(x*x+y*y+z*z);
     *theta = acos(z/(*r));
     *phi = atan2 (y,x);
    if (*phi < 0.0)
      *phi = *phi + 2.0*M_PI;
  }
}
