//=============================================================================================================
/**
* @file     layoutmaker.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     September, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implementation of the LayoutMaker class
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "layoutmaker.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QVector>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LayoutMaker::LayoutMaker()
{
}


//*************************************************************************************************************

bool LayoutMaker::makeLayout(const QList<QVector<double> > &inputPoints,
                             QList<QVector<double> > &outputPoints,
                             const QStringList &names,
                             QFile &outFile,
                             bool do_fit,
                             float prad,
                             float w,
                             float h,
                             bool writeFile)
{
    /*
    * Automatically make a layout according to the
    * channel locations in inputPoints
    */
    int         res = FAIL;
    VectorXf    r0(3);
    VectorXf    rr(3);
    float       rad,th,phi;

    float       xmin,xmax,ymin,ymax;
    int         k;
    int         neeg = inputPoints.size();
    int         nchan = inputPoints.size();

    MatrixXf rrs(neeg,3);
    VectorXf xx(neeg);
    VectorXf yy(neeg);

    if (neeg <= 0) {
        std::cout<<"No input points to lay out.";
        return false;
    }

    //Fill matrix with 3D points
    for(k = 0; k<neeg; k++) {
        rrs(k,0) = inputPoints.at(k)[0]; //x
        rrs(k,1) = inputPoints.at(k)[1]; //y
        rrs(k,2) = inputPoints.at(k)[2]; //z
    }

    std::cout<<"Channels found for layout: "<<neeg<<std::endl;

    //Fit to sphere if wanted by the user
    if (!do_fit)
        std::cout<<"Using default origin:"<<r0[0]<<r0[1]<<r0[2]<<std::endl;
    else {
        if(fit_sphere_to_points(rrs,neeg,0.05,r0,rad) == FAIL) {
            std::cout<<"Using default origin:"<<r0[0]<<r0[1]<<r0[2]<<std::endl;
        }
        else{
            std::cout<<"best fitting sphere:"<<std::endl;
            std::cout<<"torigin: "<<r0[0]<<r0[1]<<r0[2]<<rad<<std::endl<<"tradius: "<<rad<<std::endl;
        }
    }

    /*
    * Do the azimuthal equidistant projection
    */
    for (k = 0; k < neeg; k++) {
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

    for(k = 1; k < neeg; k++) {
        if (xx[k] > xmax)
            xmax = xx[k];
        else if (xx[k] < xmin)
            xmin = xx[k];
        if (yy[k] > ymax)
            ymax = yy[k];
        else if (yy[k] < ymin)
            ymin = yy[k];
    }

//    if(xmin == xmax || ymin == ymax) {
//        std::cout<<"Cannot make a layout. All positions are identical"<<std::endl;
//        return res;
//    }

    xmax = xmax + 0.6*w;
    xmin = xmin - 0.6*w;

    ymax = ymax + 0.6*h;
    ymin = ymin - 0.6*h;

    /*
    * Compose the viewports
    */
    QVector<double> point;

    for(k = 0; k < neeg; k++) {
        point.clear();
        point.append(xx[k]-0.5*w);
        point.append(-(yy[k]-0.5*h)); //rotate 180 - mirror y axis
        outputPoints.append(point);
    }

    /*
    * Write to file
    */
    if(writeFile) {
        if (!outFile.open(QIODevice::WriteOnly)) {
            std::cout<<"could not open output file";
            qDebug()<<"could not open output file";
            return FAIL;
        }

        QTextStream out(&outFile);

        for (k = 0; k < neeg; k++)
            out << k+1 << " " << xx[k]-0.5*w << " " << -(yy[k]-0.5*h) << " " << w << " " << h << " " << names.at(k)<<endl;

        std::cout<<"success while wrtiting to output file";

        outFile.close();
    }

    res = OK;

    return res;
}


//*************************************************************************************************************

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

//*************************************************************************************************************


int LayoutMaker::report_func(int loop,
                             const VectorXf &fitpar,
                             int npar,
                             double fval)
{
    /*
    * Report periodically
    */
    VectorXf r0 = fitpar;

    std::cout<<"loop: "<<loop<<"r0: "<<1000*r0[0]<<1000*r0[1]<<1000*r0[2]<<"fval: "<<fval<<std::endl;

    return OK;
}


//*************************************************************************************************************

float LayoutMaker::fit_eval(const VectorXf &fitpar,
                          int   npar,
                          void  *user_data)
{
    /*
    * Calculate the cost function value
    * Optimize for the radius inside here
    */
    fitUser user = (fitUser)user_data;
    VectorXf r0 = fitpar;
    VectorXf diff(3);
    int   k;
    float sum,sum2,one,F;

    for (k = 0, sum = sum2 = 0.0; k < user->np; k++) {
        diff = r0 - static_cast<VectorXf>(user->rr.row(k));
        one = sqrt(pow(diff(0),2) + pow(diff(1),2) + pow(diff(2),2));
        sum  += one;
        sum2 += one*one;
    }
    F = sum2 - sum*sum/user->np;

    if(user->report)
        std::cout<<"r0: "<<1000*r0[0]<<1000*r0[1]<<1000*r0[2]<<"R: "<<1000*sum/user->np<<"fval: "<<F<<std::endl;

    return F;
}


//*************************************************************************************************************

float LayoutMaker::opt_rad(VectorXf &r0,fitUser user)
{
  float sum, one;
  VectorXf diff(3);
  int   k;

  for (k = 0, sum = 0.0; k < user->np; k++) {
    diff = r0 - static_cast<VectorXf>(user->rr.row(k));
    one = sqrt(pow(diff(0),2) + pow(diff(1),2) + pow(diff(2),2));
    sum  += one;
  }

  return sum/user->np;
}


//*************************************************************************************************************

void LayoutMaker::calculate_cm_ave_dist(MatrixXf &rr,
                                        int np,
                                        VectorXf &cm,
                                        float &avep)
{
    int k,q;
    float ave;
    VectorXf diff(3);

    for (q = 0; q < 3; q++)
        cm[q] = 0.0;

    for (k = 0; k < np; k++)
        for (q = 0; q < 3; q++)
            cm[q] += rr(k,q);

    if (np > 0) {
        for (q = 0; q < 3; q++)
        cm[q] = cm[q]/np;

        for (k = 0, ave = 0.0; k < np; k++) {
            for (q = 0; q < 3; q++)
                diff[q] = rr(k,q) - cm[q];
            ave += sqrt(pow(diff(0),2) + pow(diff(1),2) + pow(diff(2),2));
        }
        avep = ave/np;
    }
}


//*************************************************************************************************************

MatrixXf LayoutMaker::make_initial_simplex(VectorXf &pars,
                                        int    npar,
                                        float  size)
{
    /*
    * Make the initial tetrahedron
    */
    MatrixXf simplex(npar+1,npar);
    int k;

    for (k = 0; k < npar+1; k++)
        simplex.row(k) = pars;

    for (k = 1; k < npar+1; k++)
        simplex(k,k-1) = simplex(k,k-1) + size;

    return simplex;
}


//*************************************************************************************************************

int LayoutMaker::fit_sphere_to_points(MatrixXf &rr,
                                     int   np,
                                     float simplex_size,
                                     VectorXf &r0,
                                     float &R)
{
    /*
    * Find the optimal sphere origin
    */
    fitUserRec user;
    float      ftol            = 1e-3;
    int        max_eval        = 5000;
    int        report_interval = -1;
    int        neval;
    MatrixXf   init_simplex;
    VectorXf   init_vals(4);

    VectorXf   cm(3);
    float      R0;
    int        k;

    int        res = FAIL;

    user.rr = rr;
    user.np = np;

    R0 = 0.1;
    calculate_cm_ave_dist(rr,np,cm,R0);

    init_simplex = make_initial_simplex(cm,3,simplex_size);

    std::cout << "sphere origin calcuated" << cm[0] << " " << cm[1] << " " << cm[2] << std::endl;

    user.report = FALSE;

    for (k = 0; k < 4; k++)
        init_vals[k] = fit_eval(static_cast<VectorXf>(init_simplex.row(k)),3,&user);

    user.report = FALSE;

    //Start the minimization
    if(MinimizerSimplex::mne_simplex_minimize(init_simplex, /* The initial simplex */
                            init_vals,                      /* Function values at the vertices */
                            3,                              /* Number of variables */
                            ftol,                           /* Relative convergence tolerance */
                            fit_eval,                       /* The function to be evaluated */
                            &user,                          /* Data to be passed to the above function in each evaluation */
                            max_eval,                       /* Maximum number of function evaluations */
                            neval,                          /* Number of function evaluations */
                            report_interval,                /* How often to report (-1 = no_reporting) */
                            report_func) != OK)             /* The function to be called when reporting */
        return FALSE;

    r0[0] = init_simplex(0,0);
    r0[1] = init_simplex(0,1);
    r0[2] = init_simplex(0,2);
    R = opt_rad(r0,&user);

    res = OK;

    return res;
}
