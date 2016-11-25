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
* @brief    Implementation of the MinimizerSimplex class
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "minimizersimplex.h"


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <iostream>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MinimizerSimplex::MinimizerSimplex()
{
}


//*************************************************************************************************************

bool MinimizerSimplex::mne_simplex_minimize(    MatrixXf& p,
                                                VectorXf& y,
                                                float ftol,
                                                float (*func)(  const VectorXf &x,
                                                                const void *user_data),
                                                const void  *user_data,
                                                int   max_eval,
                                                int   &neval,
                                                int   report,
                                                bool   (*report_func)(int loop,
                                                        const VectorXf &fitpar,
                                                        double fval))
{
    int   ndim = p.cols();  /* Number of variables */
    int   i,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,rtol;
    VectorXf psum(ndim);
    bool  result = true;
    int   count = 0;
    int   loop  = 1;

    neval = 0;
    psum = p.colwise().sum();

    if (report_func != NULL && report > 0) {
        report_func(0,static_cast<VectorXf>(p.row(0)),-1.0);
    }

    for (;;count++,loop++) {
        ilo = 1;
        ihi  =  y[1]>y[2] ? (inhi = 2,1) : (inhi = 1,2);
        for (i = 0; i < mpts; i++) {
            if (y[i]  <  y[ilo])
                ilo = i;
            if (y[i] > y[ihi]) {
                inhi = ihi;
                ihi = i;
            } else if (y[i] > y[inhi])
                if (i !=  ihi)
                    inhi = i;
        }
        rtol = 2.0*fabs(y[ihi]-y[ilo])/(fabs(y[ihi])+fabs(y[ilo]));
        /*
        * Report that we are proceeding...
        */
        if (count == report && report_func != NULL) {
            if (!report_func(loop,static_cast<VectorXf>(p.row(ilo)),y[ilo])) {
                std::cout<<"Interation interrupted.";
                result = false;
                break;
            }
            count = 0;
        }
        if (rtol < ftol)
            break;
        if (neval >=  max_eval) {
            std::cout<<"Maximum number of evaluations exceeded.";
            result  =  false;
            break;
        }
        ytry = tryit(p,y,psum,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            tryit(p,y,psum,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit(p,y,psum,func,user_data,ihi,neval,BETA);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        psum = 0.5 * ( p.row(i) + p.row(ilo) );
                        p.row(i) = psum;
                        y[i] = (*func)(psum,user_data);
                    }
                }
                neval +=  ndim;
                psum = p.colwise().sum();
            }
        }
    }

    return result;
}


//*************************************************************************************************************

float MinimizerSimplex::tryit(MatrixXf& p,
                              VectorXf& y,
                              VectorXf& psum,
                              float (*func)(const VectorXf &x, const void *user_data),
                              const void  *user_data,
                              int   ihi,
                              int &neval,
                              float fac)
{
    int ndim = p.cols();
    float fac1,fac2,ytry;
    VectorXf ptry(ndim);

    fac1 = (1.0-fac)/ndim;
    fac2 = fac1-fac;

    ptry = psum * fac1 - p.row(ihi).transpose() * fac2;

    ytry = (*func)(ptry,user_data);
    ++neval;

    if (ytry < y[ihi]) {
        y[ihi] = ytry;

        psum += ptry - p.row(ihi).transpose();
        p.row(ihi) = ptry;
    }

    return ytry;
}

#undef ALPHA
#undef BETA
#undef GAMMA
