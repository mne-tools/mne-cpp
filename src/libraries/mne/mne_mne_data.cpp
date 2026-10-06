//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_mne_data.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    Implementation of @ref MNELIB::MNEMneData.
 *
 * Provides the constructors, resize helpers and explicit deallocation
 * needed because the original C code expected manual lifecycle
 * management of the scratch buffers.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_mne_data.h"
#include "mne_inverse_operator.h"

#include <math/numerics.h>

#include <algorithm>
#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MNEMneData MNEMneData::compute(const MNEInverseOperator& inv, const MatrixXd& data, double snr)
{
    MNEMneData mne;
    mne.computeRegularization(inv, data);
    mne.selectRegularization(inv, snr);
    mne.computePredicted(inv);
    return mne;
}

//=============================================================================================================

void MNEMneData::computeRegularization(const MNEInverseOperator& inv, const MatrixXd& data)
{
    const MatrixXd white = inv.whitener * inv.proj * data;
    datap = inv.eigen_fields->data * white;

    // Projected-out channels give zero noise eigenvalues; only the others count.
    const int rank = inv.noise_cov->diag ? inv.noise_cov->dim : static_cast<int>((inv.noise_cov->eig.array() > 0).count());
    const int ncomp = std::min(rank, static_cast<int>(inv.sing.size()));
    SNR = white.colwise().squaredNorm().transpose() / rank;

    const ArrayXd sing2 = inv.sing.head(ncomp).array().square();
    const double limit = UTILSLIB::Numerics::chi2Isf(1e-3, ncomp - 1);
    lambda2_est.resize(SNR.size());
    for (Index t = 0; t < SNR.size(); ++t) {
        if (SNR(t) < 1.0) {
            lambda2_est(t) = 100.0;
            continue;
        }
        const ArrayXd alpha = datap.col(t).head(ncomp).array();
        double lambda2t = 10.0;
        for (int iter = 0; iter <= 1000; ++iter) {
            const ArrayXd error = (sing2 > 0.0).select(alpha * lambda2t / (sing2 + lambda2t), alpha);
            if (error.square().sum() < limit) {
                break;
            }
            lambda2t *= 0.9;
        }
        lambda2_est(t) = lambda2t;
    }
}

//=============================================================================================================

void MNEMneData::selectRegularization(const MNEInverseOperator& inv, double snr)
{
    if (snr > 0.0) {
        lambda2 = VectorXd::Constant(lambda2_est.size(), inv.sing.squaredNorm() / inv.nchan / snr);
    } else {
        lambda2 = lambda2_est;
    }
}

//=============================================================================================================

void MNEMneData::computePredicted(const MNEInverseOperator& inv)
{
    const ArrayXd sing2 = inv.sing.array().square();
    MatrixXd weighted(datap.rows(), datap.cols());
    for (Index t = 0; t < datap.cols(); ++t) {
        weighted.col(t) = datap.col(t).array() * sing2 / (sing2 + lambda2(t));
    }
    const MatrixXd white = inv.eigen_fields->data.transpose() * weighted;
    if (inv.noise_cov->diag) {
        predicted = inv.noise_cov->data.col(0).cwiseSqrt().asDiagonal() * white;
    } else {
        // Colorer C^(1/2) = eigvec^T sqrt(eig), the rows of eigvec being the eigenvectors.
        predicted = inv.noise_cov->eigvec.transpose() * (inv.noise_cov->eig.cwiseMax(0.0).cwiseSqrt().asDiagonal() * white);
    }
}
