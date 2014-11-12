//=============================================================================================================
/**
* @file     minimizersimplex.h
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    MinimizerSimplex class declaration.
*
*/

#ifndef MINIMIZERSIMPLEX_H
#define MINIMIZERSIMPLEX_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"
#include <iostream>

//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Eigen>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINES
//=============================================================================================================

#define ALPHA 1.0
#define BETA 0.5
#define GAMMA 2.0


//=============================================================================================================
/**
* Simplex minimizer code from numerical recipes.
*
* @brief Simplex minimizer code from numerical recipes.
*/
class MinimizerSimplex
{
public:
    //=========================================================================================================
    /**
    * Constructs a MinimizerSimplex object.
    */
    MinimizerSimplex();

    static int mne_simplex_minimize(MatrixXf p,                         /* The initial simplex */
                 VectorXf y,                                            /* Function values at the vertices */
                 int   ndim,                                            /* Number of variables */
                 float ftol,                                            /* Relative convergence tolerance */
                 float (*func)(VectorXf &x,
                         int npar,
                         void *user_data),                              /* The function to be evaluated */
                 void  *user_data,                                      /* Data to be passed to the above function in each evaluation */
                 int   max_eval,                                        /* Maximum number of function evaluations */
                 int   &neval,                                          /* Number of function evaluations */
                 int   report,                                          /* How often to report (-1 = no_reporting) */
                 int   (*report_func)(int loop,
                              VectorXf &fitpar,
                              int npar,
                              double fval));                            /* The function to be called when reporting */

private:
    static float tryit(MatrixXf p,
                 VectorXf y,
                 VectorXf psum,
                 int   ndim,
                 float (*func)(VectorXf &x,
                               int npar,
                               void *user_data),                        /* The function to be evaluated */
                 void  *user_data,                                      /* Data to be passed to the above function in each evaluation */
                 int   ihi,
                 int &neval,
                 float fac);

};

} //NAMESPACE

#endif // MINIMIZERSIMPLEX_H
