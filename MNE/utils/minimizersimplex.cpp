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
// Qt INCLUDES
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

MinimizerSimplex::MinimizerSimplex()
{
}


//*************************************************************************************************************

int MinimizerSimplex::mne_simplex_minimize(MatrixXf p,
                                           VectorXf y,
                                           int   ndim,
                                           float ftol,
                                           float (*func)(const VectorXf &x,
                                                         int npar,
                                                         void *user_data),
                                           void  *user_data,
                                           int   max_eval,
                                           int   &neval,
                                           int   report,
                                           int   (*report_func)(int loop,
                                                        const VectorXf &fitpar,
                                                        int npar,
                                                        double fval))
{
    int   i,j,ilo,ihi,inhi;
    int   mpts = ndim+1;
    float ytry,ysave,sum,rtol;
    VectorXf psum(ndim);
    int   result = 0;
    int   count = 0;
    int   loop  = 1;

    neval = 0;
    for (j = 0; j < ndim; j++) {
        for (i = 0,sum = 0.0; i<mpts; i++)
            sum +=  p(i,j);
        psum[j] = sum;
    }
    if (report_func != NULL && report > 0)
        (void)report_func(0,static_cast<VectorXf>(p.row(0)),ndim,-1.0);

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
            if (report_func(loop,static_cast<VectorXf>(p.row(ilo)),ndim,y[ilo])) {
                std::cout<<"Interation interrupted.";
                result = -1;
                break;
            }
            count = 0;
        }
        if (rtol < ftol)
            break;
        if (neval >=  max_eval) {
            std::cout<<"Maximum number of evaluations exceeded.";
            result  =  -1;
            break;
        }
        ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit(p,y,psum,ndim,func,user_data,ihi,neval,BETA);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        for (j = 0; j < ndim; j++) {
                            psum[j] = 0.5*(p(i,j)+p(ilo,j));
                            p(i,j) = psum[j];
                        }
                        y[i] = (*func)(psum,ndim,user_data);
                    }
                }
                neval +=  ndim;
                for (j = 0; j < ndim; j++) {
                    for (i = 0,sum = 0.0; i < mpts; i++)
                        sum +=  p(i,j);
                    psum[j] = sum;
                }
            }
        }
    }

    return (result);
}


//*************************************************************************************************************

float MinimizerSimplex::tryit(MatrixXf p,
                              VectorXf y,
                              VectorXf psum,
                              int   ndim,
                              float (*func)(const VectorXf &x,int npar,void *user_data),
                              void  *user_data,
                              int   ihi,
                              int &neval,
                              float fac)
{
    int j;
    float fac1,fac2,ytry;
    VectorXf ptry(ndim);

    fac1 = (1.0-fac)/ndim;
    fac2 = fac1-fac;

    for (j = 0; j < ndim; j++)
        ptry[j] = psum[j]*fac1-p(ihi,j)*fac2;

    ytry = (*func)(ptry,ndim,user_data);
    ++neval;

    if (ytry < y[ihi]) {
        y[ihi] = ytry;

        for (j = 0; j < ndim; j++) {
            psum[j] +=  ptry[j]-p(ihi,j);
            p(ihi,j) = ptry[j];
        }
    }

    return ytry;
}

#undef ALPHA
#undef BETA
#undef GAMMA

