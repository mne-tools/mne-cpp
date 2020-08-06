//=============================================================================================================
/**
 * @file     icp.h
 * @author   Ruben Dörfel <doerfelruben@aol.com>
 * @since    0.1.0
 * @date     July, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Ruben Dörfel. All rights reserved.
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
 * @brief     ICP class declaration.
 *
 */

#ifndef ICP_H
#define ICP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "rtprocessing_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================
namespace FIFFLIB{
    class FiffCoordTrans;
}
//=============================================================================================================
// DEFINE NAMESPACE NAMESPACE
//=============================================================================================================

namespace RTPROCESSINGLIB {

//=============================================================================================================
// NAMESPACE FORWARD DECLARATIONS
//=============================================================================================================

const Eigen::VectorXf vecDefaultWeigths;

//=========================================================================================================
/**
 * The ICP algorithm to register a point set with pointset coresponding to a surface.
 *
 * @param [in]  matSrcPoint         The points on the surface.
 * @param [in]  matDstPoint         The destination point set to be registrated.
 * @param [out] matTrans            The forward transformation matrix.
 * @param [in]  matTransInit        The initial forward transformation matrix.
 * @param [in]  iNumIter            The maximum number of iterations for the icp algorithms, defaults to 20.
 * @param [in]  fTol                The destination point set to be reistrated.
 *
 *
 * @return Wether the registration was succesfull.
 */

RTPROCESINGSHARED_EXPORT bool icp(const Eigen::Matrix3f& matSrcPoint,
                                  const Eigen::Matrix3f& matDstPoint,
                                  Eigen::Matrix4f& matTrans,
                                  const Eigen::Matrix4f& matTransInit = Eigen::Matrix4f::Identity(4,4),
                                  const int iNumIter = 20,
                                  const float fTol = 0.001);

//=========================================================================================================

/**
 * Corresponding point set registration using quaternions.
 *
 * @param [in]  matSrcPoint         The source point set.
 * @param [in]  matDstPoint         The destination point set.
 * @param [out] matTrans            The forward transformation matrix.
 * @param [out] fScale              The scaling parameter.
 * @param [in]  bScale              Wether to apply scaling or not. Should be false for matching data sets.
 * @param [in]  vecWeitgths         The weitghts to apply.
 *
 * @return Wether the matching was succesfull.
 */

RTPROCESINGSHARED_EXPORT bool fitMatched(const Eigen::Matrix3f& matSrcPoint,
                                         const Eigen::Matrix3f& matDstPoint,
                                         Eigen::Matrix4f& matTrans,
                                         float fScale = 1.0,
                                         const bool bScale=false,
                                         const Eigen::VectorXf& vecWeitgths = vecDefaultWeigths);

//=========================================================================================================

/**
 * Get the closest points on a surface.
 *
 * @param[in]   matR                Set of pionts, which are to be projectied.
 * @param[in]   iNP                 The number of points
 * @param[out]  matRTri             The set of points on the surface
 * @param[out]  vecNearest          Triangle of the new point
 * @param[out]  vecDist             The Distance between matR and matRTri
 *
 * @return Wether the function was succesfull.
 */

RTPROCESINGSHARED_EXPORT bool closestPointOnSurface(const Eigen::Matrix3f& matR,
                                                    const int iNP,
                                                    Eigen::Matrix3f& matRTri,
                                                    Eigen::VectorXi& vecNearest,
                                                    Eigen::VectorXf& vecDist);

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class RTPROCESINGSHARED_EXPORT ICP
{

public:
    typedef QSharedPointer<ICP> SPtr;            /**< Shared pointer type for ICP. */
    typedef QSharedPointer<const ICP> ConstSPtr; /**< Const shared pointer type for ICP. */

    //=========================================================================================================
    /**
    * Constructs a ICP object.
    */
    ICP();



protected:

private:

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace

#endif // ICP_H
