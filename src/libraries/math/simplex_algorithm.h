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
/**
 * Simplex algorithm is an optimization method to solve linear optimization problems.
 *
 * @brief Simplex algorithm.
 * @note  Implements the Strategy pattern — the cost function and report function
 *        are injected as callable template parameters (zero-overhead type erasure).
 */
class MATHSHARED_EXPORT SimplexAlgorithm
{

protected:
    //=========================================================================================================
    /**
     * Protected constructor to make class non-instantiable.
     */
    SimplexAlgorithm();

public:
    //=========================================================================================================
    /**
     * mne_simplex_fit.c
     * Refactored: mne_simplex_minimize
     *
     * Minimization with the simplex algorithm.
     * Modified from Numerical Recipes. Supports an optional absolute spatial
     * tolerance (stol) to detect simplex collapse.
     *
     * @tparam T              Scalar type (float or double).
     * @tparam CostFunc       Callable with signature T(const VectorX<T>&). Evaluates the cost at a simplex vertex.
     * @tparam ReportFunc     Callable with signature bool(int loop, const VectorX<T>& fitpar, double fval_lo, double fval_hi, double par_diff).
     *
     * @param[in,out] p       The initial simplex (npar+1 x npar). On return, row 0 is the best vertex.
     * @param[in,out] y       Function values at the vertices.
     * @param[in] ftol        Relative convergence tolerance.
     * @param[in] stol        Absolute spatial convergence tolerance (0 to disable).
     * @param[in] func        The cost function to be evaluated.
     * @param[in] max_eval    Maximum number of function evaluations.
     * @param[out] neval      Number of function evaluations performed.
     * @param[in] report      How often to report (-1 = no reporting).
     * @param[in] report_func The function to be called when reporting (may be nullptr).
     *
     * @return True when minimization succeeded, false otherwise.
     */
    template <typename T, typename CostFunc, typename ReportFunc>
    static bool simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                 Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                 T ftol,
                                 T stol,
                                 CostFunc&& func,
                                 int max_eval,
                                 int &neval,
                                 int report,
                                 ReportFunc&& report_func);

    //=========================================================================================================
    /**
     * Overload without report function (no reporting).
     */
    template <typename T, typename CostFunc>
    static bool simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                 Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                 T ftol,
                                 T stol,
                                 CostFunc&& func,
                                 int max_eval,
                                 int &neval);

private:

    template <typename T, typename CostFunc>
    static T tryit(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> &p,
                   Eigen::Matrix<T,Eigen::Dynamic, 1> &y,
                   Eigen::Matrix<T,Eigen::Dynamic, 1> &psum,
                   CostFunc&& func,
                   int   ihi,
                   int &neval,
                   T fac);
};

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

template <typename T, typename CostFunc>
T SimplexAlgorithm::tryit(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> &p,
                          Eigen::Matrix<T,Eigen::Dynamic, 1> &y,
                          Eigen::Matrix<T,Eigen::Dynamic, 1> &psum,
                          CostFunc&& func,
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

    ytry = func(ptry);
    ++neval;

    if (ytry < y[ihi]) {
        y[ihi] = ytry;

        psum += ptry - p.row(ihi).transpose();
        p.row(ihi) = ptry;
    }

    return ytry;
}

//=============================================================================================================

template <typename T, typename CostFunc>
bool SimplexAlgorithm::simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                        Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                        T ftol,
                                        T stol,
                                        CostFunc&& func,
                                        int max_eval,
                                        int &neval)
{
    auto no_report = [](int, const Eigen::Matrix<T,Eigen::Dynamic,1>&, double, double, double) { return true; };
    return simplex_minimize<T>(p, y, ftol, stol, std::forward<CostFunc>(func), max_eval, neval, -1, no_report);
}

//=============================================================================================================

template <typename T, typename CostFunc, typename ReportFunc>
bool SimplexAlgorithm::simplex_minimize(Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic>& p,
                                        Eigen::Matrix<T,Eigen::Dynamic, 1>& y,
                                        T ftol,
                                        T stol,
                                        CostFunc&& func,
                                        int max_eval,
                                        int &neval,
                                        int report,
                                        ReportFunc&& report_func)
{
    constexpr int MIN_STOL_LOOP = 5;
    int   ndim = p.cols();
    int   i,ilo,ihi,inhi;
    int   mpts = ndim+1;
    T ytry,ysave,rtol;
    double dsum;
    Eigen::Matrix<T,Eigen::Dynamic, 1> psum(ndim);
    bool  result = true;
    int   count = 0;
    int   loop  = 1;

    neval = 0;
    psum = p.colwise().sum();

    constexpr T kAlpha = static_cast<T>(1.0);
    constexpr T kBeta  = static_cast<T>(0.5);
    constexpr T kGamma = static_cast<T>(2.0);

    if (report > 0)
        report_func(0, static_cast<Eigen::Matrix<T,Eigen::Dynamic, 1>>(p.row(0)), -1.0, -1.0, 0.0);

    dsum = 0.0;
    for (;;count++,loop++) {
        ilo = 1;
        ihi  =  y[1]>y[2] ? (inhi = 2,1) : (inhi = 1,2);
        for (i = 0; i < mpts; i++) {
            if (y[i]  <  y[ilo]) ilo = i;
            if (y[i] > y[ihi]) {
                inhi = ihi;
                ihi = i;
            } else if (y[i] > y[inhi])
                if (i !=  ihi) inhi = i;
        }
        rtol = 2.0*std::fabs(y[ihi]-y[ilo])/(std::fabs(y[ihi])+std::fabs(y[ilo]));
        /*
         * Report that we are proceeding...
         */
        if (count == report) {
            if (!report_func(loop, static_cast<Eigen::Matrix<T,Eigen::Dynamic, 1>>(p.row(ilo)),
                             y[ilo], y[ihi], std::sqrt(dsum))) {
                qWarning("Iteration interrupted.");
                result = false;
                break;
            }
            count = 0;
        }
        if (rtol < ftol) break;
        if (neval >=  max_eval) {
            qWarning("Maximum number of evaluations exceeded.");
            result  =  false;
            break;
        }
        if (stol > 0) {  /* Has the simplex collapsed? */
            dsum = (p.row(ilo) - p.row(ihi)).squaredNorm();
            if (loop > MIN_STOL_LOOP && std::sqrt(dsum) < stol)
                break;
        }
        ytry = tryit<T>(p,y,psum,func,ihi,neval,-kAlpha);
        if (ytry <= y[ilo])
            tryit<T>(p,y,psum,func,ihi,neval,kGamma);
        else if (ytry >= y[inhi]) {
            ysave = y[ihi];
            ytry = tryit<T>(p,y,psum,func,ihi,neval,kBeta);
            if (ytry >= ysave) {
                for (i = 0; i < mpts; i++) {
                    if (i !=  ilo) {
                        psum = static_cast<T>(0.5) * (p.row(i) + p.row(ilo));
                        p.row(i) = psum;
                        y[i] = func(psum);
                    }
                }
                neval +=  ndim;
                psum = p.colwise().sum();
            }
        }
    }
    return result;
}

} //NAMESPACE

#endif // SIMPLEXALGORITHM_H
