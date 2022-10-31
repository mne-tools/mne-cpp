//=============================================================================================================
/**
 * @file     simplex_algorithm.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
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
 * @brief    SimplexAlgorithm Template Implementation.
 *
 */

#ifndef SIMPLEXALGORITHM_H
#define SIMPLEXALGORITHM_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * Simplex algorithm is an optimization method to solve linear optimization problems.
 *
 * @brief Simplex algorithm
 */
class UTILSSHARED_EXPORT SimplexAlgorithm
{

protected:
    //=========================================================================================================
    /**
     * Protected Constrcutor to make class non-instantiable.
     */
    SimplexAlgorithm();

public:
    //=========================================================================================================
    /**
     * mne_simplex_fit.c
     * Refactored: mne_simplex_minimize
     *
     * Minimization with the simplex algorithm. Float implementation
     * Modified from Numerical recipes
     *
     * @param[in] p              The initial simplex.
     * @param[in] y              Function values at the vertices.
     * @param[in] ftol           Relative convergence tolerance.
     * @param[in] func           The function to be evaluated.
     * @param[in] user_data      Data to be passed to the above function in each evaluation.
     * @param[in] max_eval       Maximum number of function evaluations.
     * @param[in] neval          Number of function evaluations.
     * @param[in] report         How often to report (-1 = no_reporting).
     * @param[in] report_func    The function to be called when reporting.
     *
     * @return True when setup was successful, false otherwise.
     */
    template <typename T>
    static bool simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                 Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                 T ftol,
                                 T (*func)(const Eigen::Matrix<T,Eigen::Dynamic, 1>& x, const void *user_data),
                                 const void *user_data,
                                 int max_eval,
                                 int &neval,
                                 int report,
                                 bool (*report_func)(int loop, const Eigen::Matrix<T,Eigen::Dynamic, 1>& fitpar, double fval));

private:

    template <typename T>
    static T tryit(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> &p,
                   Eigen::Matrix<T,Eigen::Dynamic, 1> &y,
                   Eigen::Matrix<T,Eigen::Dynamic, 1> &psum,
                   T (*func)(  const Eigen::Matrix<T,Eigen::Dynamic, 1> &x,const void *user_data),                     /* The function to be evaluated */
                   const void *user_data,                                      /* Data to be passed to the above function in each evaluation */
                   int   ihi,
                   int &neval,
                   T fac);
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template <typename T>
bool SimplexAlgorithm::simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                        Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                        T ftol,
                                        T (*func)(const Eigen::Matrix<T,Eigen::Dynamic, 1>& x, const void *user_data),
                                        const void *user_data,
                                        int max_eval,
                                        int &neval,
                                        int report,
                                        bool (*report_func)(int loop, const Eigen::Matrix<T,Eigen::Dynamic, 1>& fitpar, double fval))
{
    int   ndim = p.cols();  /* Number of variables */
    int   i,ilo,ihi,inhi;
    int   mpts = ndim+1;
    T ytry,ysave,rtol;
    Eigen::Matrix<T,Eigen::Dynamic, 1> psum(ndim);
    bool  result = true;
    int   count = 0;
    int   loop  = 1;

    neval = 0;
    psum = p.colwise().sum();

    if (report_func != NULL && report > 0) {
        report_func(0,static_cast< Eigen::Matrix<T,Eigen::Dynamic, 1> >(p.row(0)),-1.0);
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
        rtol = 2.0*std::fabs(y[ihi]-y[ilo])/(std::fabs(y[ihi])+std::fabs(y[ilo]));
        /*
        * Report that we are proceeding...
        */
        if (count == report && report_func != NULL) {
            if (!report_func(loop,static_cast< Eigen::Matrix<T,Eigen::Dynamic, 1> >(p.row(ilo)),y[ilo])) {
                qCritical("Interation interrupted.");
                result = false;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (neval >=  max_eval) {
            qCritical("Maximum number of evaluations exceeded.");
            result  =  false;
            break;
        }
        ytry = tryit<T>(p,y,psum,func,user_data,ihi,neval,-ALPHA);
        if (ytry <= y[ilo])
            tryit<T>(p,y,psum,func,user_data,ihi,neval,GAMMA);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit<T>(p,y,psum,func,user_data,ihi,neval,BETA);
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

//=============================================================================================================

template <typename T>
T SimplexAlgorithm::tryit(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> &p,
                          Eigen::Matrix<T,Eigen::Dynamic, 1> &y,
                          Eigen::Matrix<T,Eigen::Dynamic, 1> &psum,
                          T (*func)(  const Eigen::Matrix<T,Eigen::Dynamic, 1> &x,const void *user_data),                     /* The function to be evaluated */
                          const void *user_data,                                      /* Data to be passed to the above function in each evaluation */
                          int   ihi,
                          int &neval,
                          T fac)
{
    int ndim = p.cols();
    T fac1,fac2,ytry;

    Eigen::Matrix<T,Eigen::Dynamic, 1> ptry(ndim);

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
} //NAMESPACE

#undef ALPHA
#undef BETA
#undef GAMMA

#endif // SIMPLEXALGORITHM_H
