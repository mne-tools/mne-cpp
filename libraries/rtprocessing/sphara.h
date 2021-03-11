//=============================================================================================================
/**
 * @file     sphara.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    Declaration of the Sphara class
 *
 */

#ifndef SPHARA_RTPROCESSING_H
#define SPHARA_RTPROCESSING_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE RTPROCESSINGLIB
//=============================================================================================================

namespace RTPROCESSINGLIB
{

//=============================================================================================================
// DEFINES
//=============================================================================================================

//=========================================================================================================
/**
 * Constructs a SPHARA operator.
 *
 * @param[in] matBaseFct        The SPHARA basis functions.
 * @param[in] vecIndices        The indices of the positions in the final oeprator which are to be filled with the basis functions weights (i.e. these indices could respond to the indices of gradioemteres in a VectorView system).
 * @param[in] iOperatorDim      The dimensions of the final SPHARA operator. Make sure that these correspond to the dimensions of the data matrix you want tol multiply with the SPHARA operator.
 * @param[in] iNBaseFct         The number of SPHARA basis functions to take.
 * @param[in] iSkip             The value to skip when reading the vecIndices variabel. I.e. use this when dealing with VectorView triplets, which include two gradiometers.
 *
 * @return Returns the final SPHARA operator with dimensions (iOperatorDim,iOperatorDim).
 */
RTPROCESINGSHARED_EXPORT Eigen::MatrixXd makeSpharaProjector(const Eigen::MatrixXd& matBaseFct,
                                                             const Eigen::VectorXi& vecIndices,
                                                             int iOperatorDim,
                                                             int iNBaseFct,
                                                             int iSkip = 0);

} // NAMESPACE RTPROCESSINGLIB

#endif // SPHARA_RTPROCESSING_H
