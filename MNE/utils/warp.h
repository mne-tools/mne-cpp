//=============================================================================================================
/**
* @file     mnemath.h
* @author   Jana Kiesel <jana.kiesel@tu-ilmenau.de>
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
*
* @section  LICENSE
*
* Copyright (C) 2015, Jana Kiesel and Matti Hamalainen. All rights reserved.
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
* @brief    warp class declaration.
*
*/

#ifndef WARP_H
#define WARP_H

#endif // WARP_H

//*************************************************************************************************************
//=============================================================================================================
// MNE INCLUDES
//=============================================================================================================

#include "utils_global.h"

//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
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
// FORWARD DECLARATIONS
//=============================================================================================================


//=============================================================================================================
/**
* @brief Thin Plate Spline Warp
*/
class UTILSSHARED_EXPORT Warp
{
public:

    typedef QSharedPointer<Warp> SPtr;            /**< Shared pointer type for Warp. */
    typedef QSharedPointer<const Warp> ConstSPtr; /**< Const shared pointer type for Warp. */

    //=========================================================================================================
    /**
    * Calculates the TPS Warp of given setup
    *
    * @param[in] srcLm      3D Landmarks of the source geometry
    * @param[in] dstLm      3D Landmarks of the destination geometry
    * @param[in] srcVert    Vertices of the source geometry
    *
    * @return dstVert   Vertices of the warped destination geometry
    */
    MatrixXd calculate(const MatrixXd & srcLm, const MatrixXd & dstLm, const MatrixXd & srcVert);

    //=========================================================================================================
    /**
    * Warps the source vertices with previously calculated weightningparameters
    *
    * @param[in] srcVert    Vertices of the source geometry
    *
    * @return dstVert   Vertices of the warped destination geometry
    */
     MatrixXd calculate(const MatrixXd & srcVert);

private:
    //=========================================================================================================
    /**
    * Calculate the weighting parameters.
    *
    * @param[out] warpWeighting Weighting parameters of the tps warp
    * @param[out] polWeighting  Weighting papameters of the polynomial warp
    */
    bool calcWeighting(const MatrixXd & srcLm, const MatrixXd & dstLm);

    //=========================================================================================================
    /**
    * Warp the Vertices of the source geometry
    *
    * @return Warped Vertices
    */
    MatrixXd warpVertices(const MatrixXd srcVert);

    //=========================================================================================================

    MatrixXd warpWeighting; /** Weighting parameters of the tps Warp */
    MatrixXd polWeighting;  /** Weighting papameters of the polynomial warp */

};

} // NAMESPACE

#endif // WARP_H
