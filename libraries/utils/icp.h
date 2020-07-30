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

//=============================================================================================================
// DEFINE NAMESPACE NAMESPACE
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
// NAMESPACE FORWARD DECLARATIONS
//=============================================================================================================

const Eigen::VectorXd vecDefaultWeigths;

//=============================================================================================================
/**
 * Description of what this class is intended to do (in detail).
 *
 * @brief Brief description of this class.
 */
class ICP
{

public:
    typedef QSharedPointer<ICP> SPtr;            /**< Shared pointer type for ICP. */
    typedef QSharedPointer<const ICP> ConstSPtr; /**< Const shared pointer type for ICP. */

    //=========================================================================================================
    /**
    * Constructs a ICP object.
    */
    ICP();

    //=========================================================================================================
    /**
     * Corresponding point set registration using quaternions.
     *
     * @param [in]  matSrcPoint          The source point set.
     * @param [in]  matDstPoint          The destination point set.
     * @param [in]  vecWeitgths          The weitghts to apply.
     * @param [in]  bScale               Wether to apply scaling or not.
     * @param [out] vecTransParam        The quaternion and scale parameters (q1,q2,q3,t1,t2,t3,s).
     *
     * @return Wether the matching was succesfull.
     */
    bool fit_matched(const Eigen::MatrixXd& matSrcPoint,
                     const Eigen::MatrixXd& matDstPoint,
                     Eigen::VectorXd& vecTransParam,
                     const Eigen::VectorXd& vecWeitgths = vecDefaultWeigths,
                     const bool bScale=false);

protected:

private:

};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace

#endif // ICP_H
