//=============================================================================================================
/**
* @file     fwd_eeg_sphere_model.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     December, 2016
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
* @brief    Implementation of the FwdEegSphereModel Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fwd_eeg_sphere_model.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVERSELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FwdEegSphereModel::FwdEegSphereModel()
{

}


//*************************************************************************************************************

//FwdEegSphereModel::FwdEegSphereModel(const FwdEegSphereModel& p_FwdEegSphereModel)
//: name(p_FwdEegSphereModel.name)
//, layers(p_FwdEegSphereModel.layers)
//, r0(p_FwdEegSphereModel.r0)
//, fn(p_FwdEegSphereModel.fn)
//, nterms(p_FwdEegSphereModel.nterms)
//, mu(p_FwdEegSphereModel.mu)
//, lambda(p_FwdEegSphereModel.lambda)
//, nfit(p_FwdEegSphereModel.nfit)
//, scale_pos(p_FwdEegSphereModel.scale_pos)
//{

//}


//*************************************************************************************************************

FwdEegSphereModel::~FwdEegSphereModel()
{

}


////*************************************************************************************************************
//// fwd_multi_spherepot.c
//double FwdEegSphereModel::fwd_eeg_get_multi_sphere_model_coeff(int n)
//{
//    double **M,**Mn,**help,**Mm;
//    static double **mat1 = NULL;
//    static double **mat2 = NULL;
//    static double **mat3 = NULL;
//    static double *c1 = NULL;
//    static double *c2 = NULL;
//    static double *cr = NULL;
//    static double *cr_mult = NULL;
//    double div,div_mult;
//    double n1;
//#ifdef TEST
//    double rel1,rel2;
//    double b,c;
//#endif
//    int    k;

//    if (this->nlayer() == 0 || this->nlayer() == 1)
//        return 1.0;
//    /*
//   * Now follows the tricky case
//   */
//#ifdef TEST
//    if (this->nlayer() == 2) {
//        rel1 = layers[0].sigma/layers[1].sigma;
//        n1 = n + 1.0;
//        div_mult = 2.0*n + 1;
//        b = pow(this->layers[0].rel_rad,div_mult);
//        return div_mult/((n1 + n*rel1) + b*n1*(rel1-1.0));
//    }
//    else if (this->nlayer() == 3) {
//        rel1 = this->layers[0].sigma/this->layers[1].sigma;
//        rel2 = this->layers[1].sigma/this->layers[2].sigma;
//        n1 = n + 1.0;
//        div_mult = 2.0*n + 1.0;
//        b = pow(this->layers[0].rel_rad,div_mult);
//        c = pow(this->layers[1].rel_rad,div_mult);
//        div_mult = div_mult*div_mult;
//        div = (b*n*n1*(rel1-1.0)*(rel2-1.0) + c*(rel1*n + n1)*(rel2*n + n1))/c +
//                n1*(b*(rel1-1.0)*(rel2*n1 + n) + c*(rel1*n + n1)*(rel2-1.0));
//        return div_mult/div;
//    }
//#endif
//    if (n == 1) {
//        /*
//     * Initialize the arrays
//     */
//        c1 = REALLOC(c1,this->nlayer()-1,double);
//        c2 = REALLOC(c2,this->nlayer()-1,double);
//        cr = REALLOC(cr,this->nlayer()-1,double);
//        cr_mult = REALLOC(cr_mult,this->nlayer()-1,double);
//        for (k = 0; k < this->nlayer()-1; k++) {
//            c1[k] = this->layers[k].sigma/this->layers[k+1].sigma;
//            c2[k] = c1[k] - 1.0;
//            cr_mult[k] = this->layers[k].rel_rad;
//            cr[k] = cr_mult[k];
//            cr_mult[k] = cr_mult[k]*cr_mult[k];
//        }
//        if (mat1 == NULL)
//            mat1 = ALLOC_DCMATRIX(2,2);
//        if (mat2 == NULL)
//            mat2 = ALLOC_DCMATRIX(2,2);
//        if (mat3 == NULL)
//            mat3 = ALLOC_DCMATRIX(2,2);
//    }
//    /*
//   * Increment the radius coefficients
//   */
//    for (k = 0; k < this->nlayer()-1; k++)
//        cr[k] = cr[k]*cr_mult[k];
//    /*
//   * Multiply the matrices
//   */
//    M  = mat1;
//    Mn = mat2;
//    Mm = mat3;
//    M[0][0] = M[1][1] = 1.0;
//    M[0][1] = M[1][0] = 0.0;
//    div      = 1.0;
//    div_mult = 2.0*n + 1.0;
//    n1       = n + 1.0;

//    for (k = this->nlayer()-2; k >= 0; k--) {

//        Mm[0][0] = (n + n1*c1[k]);
//        Mm[0][1] = n1*c2[k]/cr[k];
//        Mm[1][0] = n*c2[k]*cr[k];
//        Mm[1][1] = n1 + n*c1[k];

//        Mn[0][0] = Mm[0][0]*M[0][0] + Mm[0][1]*M[1][0];
//        Mn[0][1] = Mm[0][0]*M[0][1] + Mm[0][1]*M[1][1];
//        Mn[1][0] = Mm[1][0]*M[0][0] + Mm[1][1]*M[1][0];
//        Mn[1][1] = Mm[1][0]*M[0][1] + Mm[1][1]*M[1][1];
//        help = M;
//        M = Mn;
//        Mn = help;
//        div = div*div_mult;

//    }
//    return n*div/(n*M[1][1] + n1*M[1][0]);
//}
