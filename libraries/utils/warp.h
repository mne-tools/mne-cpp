//=============================================================================================================
/**
* @file     warp.h
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
* @brief    Warp class declaration.
*
*/

#ifndef WARP_H
#define WARP_H

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
// Qt INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QStringList>


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
    * @param[in]  sLm      3D Landmarks of the source geometry
    * @param[in]  dLm      3D Landmarks of the destination geometry
    * @param[in]  sVert    Vertices of the source geometry
    *
    * @return wVert   Vertices of the warped destination geometry
    */
    MatrixXf calculate(const MatrixXf & sLm, const MatrixXf &dLm, const MatrixXf & sVert);

    //=========================================================================================================
    /**
    * Calculates the TPS Warp of a given setup for a List of vertices
    *
    * @param[in]  sLm       3D Landmarks of the source geometry
    * @param[in]  dLm       3D Landmarks of the destination geometry
    * @param[in/out] vertList  List of Vertices of the source geometry that are warped to the destination
    */
    void calculate(const MatrixXf & sLm, const MatrixXf &dLm, QList<MatrixXf> & vertList);

    //=========================================================================================================
    /**
    * Read electrode positions from MRI Database
    *
    * @param[in]  electrodeFileName    .txt file of electrodes
    *
    * @return electrodes   Matrix with electrode positions
    */
    MatrixXf readsLm(const QString &electrodeFileName);

private:

    //=========================================================================================================
    /**
    * Calculate the weighting parameters.
    *
    * @param[in]  sLm      3D Landmarks of the source geometry
    * @param[in]  dLm      3D Landmarks of the destination geometry
    * @param[out] warpWeight Weighting parameters of the tps warp
    * @param[out] polWeight  Weighting papameters of the polynomial warp
    */
    bool calcWeighting(const MatrixXf& sLm, const MatrixXf &dLm, MatrixXf& warpWeight, MatrixXf& polWeight);

    //=========================================================================================================
    /**
    * Warp the Vertices of the source geometry
    *
    * @param[in]  sVert    Vertices of the source geometry
    * @param[in]  sLm      3D Landmarks of the source geometry
    * @param[in]  warpWeight Weighting parameters of the tps warp
    * @param[in]  polWeight  Weighting papameters of the polynomial warp
    *
    * @return Warped Vertices
    */
    MatrixXf warpVertices(const MatrixXf & sVert, const MatrixXf & sLm, const MatrixXf& warpWeight, const MatrixXf& polWeight);

};

} // NAMESPACE

#endif // WARP_H
