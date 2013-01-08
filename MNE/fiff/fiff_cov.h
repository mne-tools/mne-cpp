//=============================================================================================================
/**
* @file     fiff_cov.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    Contains the FiffCov class declaration.
*
*/

#ifndef FIFF_COV_H
#define FIFF_COV_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fiff_global.h"

//*************************************************************************************************************
//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include "fiff_types.h"
#include "fiff_proj.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace FIFFLIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


//=============================================================================================================
/**
* Fiff cov data, which corresponds to a covariance data matrix
*
* @brief covariance data
*/
class FIFFSHARED_EXPORT FiffCov
{

public:
    //=========================================================================================================
    /**
    * ctor
    */
    FiffCov();

    //=========================================================================================================
    /**
    * copy ctor
    */
    FiffCov(const FiffCov* p_pFiffCov);

    //=========================================================================================================
    /**
    * Destroys the MNECov.
    */
    ~FiffCov();

public:
    fiff_int_t  kind;       /**< ToDo... */
    bool diag;              /**< ToDo... */
    fiff_int_t dim;         /**< ToDo... */
    QStringList names;      /**< ToDo... */
    MatrixXd data;         /**< ToDo... */
    QList<FiffProj> projs; /**< ToDo... */
    QStringList bads;       /**< ToDo... */
    fiff_int_t nfree;       /**< ToDo... */
    VectorXd eig;          /**< ToDo... */
    MatrixXd eigvec;       /**< ToDo... */
};

} // NAMESPACE

#endif // FIFF_COV_H
