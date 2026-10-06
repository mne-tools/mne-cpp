//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_mne_data.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Per-data-set results of a minimum-norm computation: eigenfield projections, SNR, lambda2 and predicted data.
 *
 * @ref MNELIB::MNEMneData ports MNE-C's @c mneMneDataRec together with the
 * code that fills it (@c mne_project_to_eigen_fields, @c mne_analyze's
 * @c compute_regularization / @c select_regularization and
 * @c compute_predicted_data). @ref INVLIB::InvMinimumNorm::mneData fills it
 * from its prepared inverse operator.
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

class MNEInverseOperator;

//=============================================================================================================
/**
 * Implements MNE Mne Data (Replaces *mneMneData,mneMneDataRec; struct of MNE-C mne_types.h).
 *
 * @brief Data associated with MNE computations for each mneMeasDataSet
 *
 * @snippet ex_inv_api/main.cpp mne_mne_data_usage
 */
class MNESHARED_EXPORT MNEMneData
{
public:
    typedef QSharedPointer<MNEMneData> SPtr;            /**< Shared pointer type for MNEMneData. */
    typedef QSharedPointer<const MNEMneData> ConstSPtr; /**< Const shared pointer type for MNEMneData. */

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

    //=========================================================================================================
    /**
     * Fills all fields for one data set: projects it on the eigenfields, estimates the SNR and
     * lambda2 per time point, selects the lambda2 to use and computes the predicted data.
     *
     * @param[in] inv    An inverse operator prepared with @ref MNEInverseOperator::prepare_inverse_operator.
     * @param[in] data   Measured data, one row per channel of @p inv in its order, one column per time.
     * @param[in] snr    Power SNR for a fixed lambda2 (see @ref selectRegularization), <= 0 to use lambda2_est.
     *
     * @return The MNE data.
     */
    static MNEMneData compute(const MNEInverseOperator& inv, const Eigen::MatrixXd& data, double snr);

    //=========================================================================================================
    /**
     * Projects the whitened data on the eigenfields (MNE-C @c mne_project_to_eigen_fields) and
     * estimates SNR and lambda2_est per time point (MNE-C @c compute_regularization, noise method).
     *
     * SNR is the power of the whitened data per non-zero noise eigenvalue (the square of
     * @c mne.minimum_norm.estimate_snr's @c snr). lambda2_est is 100 where SNR < 1; elsewhere it starts
     * at 10 and shrinks by 0.9 until the squared prediction error of the whitened data falls below the
     * chi^2 point at p = 0.001 with one degree of freedom less than the components used.
     *
     * @param[in] inv    A prepared inverse operator.
     * @param[in] data   Measured data, one row per channel of @p inv, one column per time.
     */
    void computeRegularization(const MNEInverseOperator& inv, const Eigen::MatrixXd& data);

    //=========================================================================================================
    /**
     * Selects lambda2 per time point (MNE-C @c select_regularization): trace_ratio / @p snr for a
     * positive power SNR, lambda2_est otherwise. trace_ratio is the mean squared singular value.
     *
     * @param[in] inv   The prepared inverse operator used for computeRegularization.
     * @param[in] snr   Power SNR, <= 0 to use lambda2_est.
     */
    void selectRegularization(const MNEInverseOperator& inv, double snr);

    //=========================================================================================================
    /**
     * Computes the data the minimum-norm estimate predicts with the selected lambda2, colored back to
     * the sensors (MNE-C @c compute_predicted_data; the data part of @c mne.minimum_norm.apply_inverse's
     * residual).
     *
     * @param[in] inv   The prepared inverse operator used for computeRegularization.
     */
    void computePredicted(const MNEInverseOperator& inv);

public:
    Eigen::MatrixXd datap;       /**< Projection of the whitened data onto the eigenfields, components x times. */
    Eigen::MatrixXd predicted;   /**< The predicted data, channels x times. */
    Eigen::VectorXd SNR;         /**< Estimated power SNR as a function of time. */
    Eigen::VectorXd lambda2_est; /**< Regularization parameter estimated from available data. */
    Eigen::VectorXd lambda2;     /**< Regularization parameter to be used (as a function of time). */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // NAMESPACE MNELIB

#endif // MNEMNEDATA_H
