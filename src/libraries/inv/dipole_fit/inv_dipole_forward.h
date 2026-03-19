//=============================================================================================================
/**
 * @file     inv_dipole_forward.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     December, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    InvDipoleForward class declaration.
 *
 */

#ifndef INV_DIPOLE_FORWARD_H
#define INV_DIPOLE_FORWARD_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../inv_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <memory>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * Implements InvDipoleForward (Replaces *dipoleForward,dipoleForwardRec struct of MNE-C fit_types.h).
 *
 * @brief Stores forward field matrices and SVD decomposition for magnetic dipole fitting.
 *
 * For each set of candidate dipole(s), the forward solution matrix A (size 3*ndip x nch) is computed,
 * column-normalized, and decomposed via SVD: A = U * diag(sing) * V^T.
 * The members uu, vv, and sing store the SVD factors used for fast goodness-of-fit evaluation
 * and dipole moment reconstruction.
 */
class INVSHARED_EXPORT InvDipoleForward
{
public:
    typedef QSharedPointer<InvDipoleForward> SPtr;              /**< Shared pointer type for InvDipoleForward. */
    typedef QSharedPointer<const InvDipoleForward> ConstSPtr;   /**< Const shared pointer type for InvDipoleForward. */
    typedef std::unique_ptr<InvDipoleForward> UPtr;             /**< Unique pointer type for InvDipoleForward. */

    //=========================================================================================================
    /**
     * Constructs a default (empty) InvDipoleForward.
     */
    InvDipoleForward() = default;

    //=========================================================================================================
    /**
     * Destructs the InvDipoleForward. Eigen members are cleaned up automatically.
     */
    ~InvDipoleForward() = default;

public:
    Eigen::MatrixXf rd;             /**< Dipole locations (ndip x 3). */
    int   ndip = 0;                 /**< Number of dipoles. */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> fwd;  /**< Forward solution, projected and whitened (3*ndip x nch, row-major). */
    Eigen::VectorXf scales;         /**< Column normalization scales applied to fwd (3*ndip). */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> uu;   /**< Right singular vectors V^T of fwd (udim x nch, row-major). */
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> vv;   /**< Left singular vectors U^T of fwd (udim x 3*ndip, row-major). */
    Eigen::VectorXf sing;           /**< Singular values of the forward matrix (3*ndip). */
    int   nch = 0;                  /**< Number of channels. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE INVLIB

#endif // INV_DIPOLE_FORWARD_H
