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
class FIFFSHARED_EXPORT FiffCov : public QSharedData
{
public:
    typedef QSharedPointer<FiffCov> SPtr;               /**< Shared pointer type for FiffCov. */
    typedef QSharedPointer<const FiffCov> ConstSPtr;    /**< Const shared pointer type for FiffCov. */
    typedef QSharedDataPointer<FiffCov> SDPtr;       /**< Shared data pointer type for FiffCov. */

    //=========================================================================================================
    /**
    * Constructs the covariance data matrix.
    */
    FiffCov();

    //=========================================================================================================
    /**
    * Copy constructor.
    *
    * @param[in] p_FiffCov   Covariance data matrix which should be copied
    */
    FiffCov(const FiffCov &p_FiffCov);

    //=========================================================================================================
    /**
    * Destroys the covariance data matrix.
    */
    ~FiffCov();

    //=========================================================================================================
    /**
    * Initializes the covariance data matrix.
    */
    void clear();

public:
    fiff_int_t  kind;       /**< Covariance kind -> fiff_constants.h */
    bool diag;              /**< If the covariance is stored in a diagonal order. */
    fiff_int_t dim;         /**< Dimension of the covariance (dim x dim). */
    QStringList names;      /**< Channel names. */
    MatrixXd data;          /**< Covariance data */
    QList<FiffProj> projs;  /**< List of available ssp projectors. */
    QStringList bads;       /**< List of bad channels. */
    fiff_int_t nfree;       /**< ToDo */
    VectorXd eig;           /**< Vector of eigenvalues. */
    MatrixXd eigvec;        /**< Matrix of eigenvectors. */
};

} // NAMESPACE

#endif // FIFF_COV_H
