//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_mne_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Container for the working tensors of an MNE inverse computation (whitened residuals, source-time matrices).
 *
 * @ref MNELIB::MNEMneData ports the @c mneMneDataRec struct and is the
 * scratchpad shared between @c prepare_inverse_operator and the
 * per-tstep solver inside @ref MNELIB::MinimumNorm. It stores the
 * whitened data, the SVD-rotated residuals and the resulting source
 * time courses without allocating per call, which matters for raw-data
 * inverse application over long recordings.
 */

#ifndef MNEMNEDATA_H
#define MNEMNEDATA_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Implements MNE Mne Data (Replaces *mneMneData,mneMneDataRec; struct of MNE-C mne_types.h).
 *
 * @brief Data associated with MNE computations for each mneMeasDataSet
 */
class MNESHARED_EXPORT MNEMneData
{
public:
    typedef QSharedPointer<MNEMneData> SPtr;              /**< Shared pointer type for MNEMneData. */
    typedef QSharedPointer<const MNEMneData> ConstSPtr;   /**< Const shared pointer type for MNEMneData. */

    //=========================================================================================================
    /**
     * Constructs the MNEMneData.
     */
    MNEMneData() = default;

    //=========================================================================================================
    /**
     * Destroys the MNEMneData.
     */
    ~MNEMneData() = default;

public:
    Eigen::MatrixXf datap;          /**< Projection of the whitened data onto the field eigenvectors. */
    Eigen::MatrixXf predicted;      /**< The predicted data. */
    Eigen::VectorXf SNR;            /**< Estimated power SNR as a function of time. */
    Eigen::VectorXf lambda2_est;    /**< Regularization parameter estimated from available data. */
    Eigen::VectorXf lambda2;        /**< Regularization parameter to be used (as a function of time). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMNEDATA_H
