//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_dipole_forward.h
 * @since 2026
 * @date  March 2026
 * @brief Per-iteration forward-field cache (forward matrix, SVD, column normalisation) used by the dipole-fit cost function.
 *
 * @ref INVLIB::InvDipoleForward replaces the @c dipoleForwardRec record
 * of MNE-C and stores, for one candidate dipole or set of dipoles, the
 * column-normalised @c (3·n_dip × n_ch) forward matrix together with
 * its singular-value decomposition @c U·diag(σ)·V^T. Holding the SVD
 * makes goodness-of-fit evaluation a single dense matmul per iteration
 * and lets the Nelder-Mead optimiser query the residual without
 * re-solving a least-squares system at every step.
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
