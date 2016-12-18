//=============================================================================================================
/**
* @file     simplex_algorithm.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    SimplexAlgorithm class declaration.
*
*/

#ifndef SIMPLEXALGORITHM_H
#define SIMPLEXALGORITHM_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
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

public:
    //=========================================================================================================
    /**
    * Protected Constrcutor to make class non-instantiable.
    */
    SimplexAlgorithm();

    //=========================================================================================================
    /**
    * mne_simplex_fit.c
    * Refactored: mne_simplex_minimize
    *
    * Minimization with the simplex algorithm. Float implementation - ToDo: fiddel with template
    * Modified from Numerical recipes
    *
    * @param[in] p              The initial simplex
    * @param[in] y              Function values at the vertices
    * @param[in] ftol           Relative convergence tolerance
    * @param[in] func           The function to be evaluated
    * @param[in] user_data      Data to be passed to the above function in each evaluation
    * @param[in] max_eval       Maximum number of function evaluations
    * @param[in] neval          Number of function evaluations
    * @param[in] report         How often to report (-1 = no_reporting)
    * @param[in] report_func    The function to be called when reporting
    *
    * @return True when setup was successful, false otherwise
    */
    static bool simplex_minimize(   Eigen::MatrixXf& p, Eigen::VectorXf& y, float ftol,
                                        float (*func)(const Eigen::VectorXf &x, const void *user_data),
                                        const void *user_data, int max_eval, int &neval, int report,
                                        bool (*report_func)(int loop, const Eigen::VectorXf &fitpar, double fval));

    //=========================================================================================================
    /**
    * mne_simplex_fit.c
    * Refactored: mne_simplex_minimize
    *
    * Minimization with the simplex algorithm. Double implementation - ToDo: fiddle with template
    * Modified from Numerical recipes
    *
    * @param[in] p              The initial simplex
    * @param[in] y              Function values at the vertices
    * @param[in] ftol           Relative convergence tolerance
    * @param[in] func           The function to be evaluated
    * @param[in] user_data      Data to be passed to the above function in each evaluation
    * @param[in] max_eval       Maximum number of function evaluations
    * @param[in] neval          Number of function evaluations
    * @param[in] report         How often to report (-1 = no_reporting)
    * @param[in] report_func    The function to be called when reporting
    *
    * @return True when setup was successful, false otherwise
    */
    static bool simplex_minimize (Eigen::MatrixXd& p, Eigen::VectorXd& y, double ftol,
                                    double (*func)( const Eigen::VectorXd &fitpar,const void *user_data),
                                    const void *user_data, int max_eval, int &neval, int report,
                                    bool (*report_func)(int loop, const Eigen::VectorXd &fitpar, double fval));

private:
    // float implementation - ToDo fiddle with template
    static float tryit( Eigen::MatrixXf& p,
                        Eigen::VectorXf& y,
                        Eigen::VectorXf& psum,
                        float (*func)(  const Eigen::VectorXf &x,
                                        const void *user_data),                     /* The function to be evaluated */
                        const void *user_data,                                      /* Data to be passed to the above function in each evaluation */
                        int   ihi,
                        int &neval,
                        float fac);

    // double implementation - ToDo fiddle with template
    static double tryit(Eigen::MatrixXd& p,
                        Eigen::VectorXd& y,
                        Eigen::VectorXd& psum,
                        double (*func)(const Eigen::VectorXd &,const void *),
                        const void   *user_data,
                        int ihi,
                        int &neval,
                        double fac);

};

} //NAMESPACE

#endif // SIMPLEXALGORITHM_H
