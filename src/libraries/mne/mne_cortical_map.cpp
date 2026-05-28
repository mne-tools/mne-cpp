//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file mne_cortical_map.cpp
 * @since April 2026
 * @brief Implementation of @ref MNELIB::MNECorticalMap.
 *
 * Implements binary read of the FreeSurfer @c .map file and the
 * construction of the per-vertex weight table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_cortical_map.h"
#include "mne_forward_solution.h"
#include "mne_inverse_operator.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_named_matrix.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNELIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd MNECorticalMap::makeCorticalMap(
    const MNEForwardSolution& fwd,
    const MNEInverseOperator& inv,
    const FiffInfo& info)
{
    Q_UNUSED(info);

    // Get the inverse kernel (nSources x nChannels)
    const MatrixXd& kernel = const_cast<MNEInverseOperator&>(inv).getKernel();
    if (kernel.rows() == 0 || kernel.cols() == 0) {
        qWarning("MNECorticalMap::makeCorticalMap - Inverse kernel is empty. "
                 "Make sure the inverse operator has been prepared (assemble_kernel).");
        return MatrixXd();
    }

    // Get the forward gain matrix (nChannels x nSources)
    if (!fwd.sol || fwd.sol->data.rows() == 0) {
        qWarning("MNECorticalMap::makeCorticalMap - Forward solution is empty.");
        return MatrixXd();
    }
    const MatrixXd& gain = fwd.sol->data;

    // Verify dimension compatibility
    if (kernel.cols() != gain.rows()) {
        qWarning("MNECorticalMap::makeCorticalMap - Dimension mismatch: "
                 "kernel is %lld x %lld, gain is %lld x %lld",
                 static_cast<long long>(kernel.rows()),
                 static_cast<long long>(kernel.cols()),
                 static_cast<long long>(gain.rows()),
                 static_cast<long long>(gain.cols()));
        return MatrixXd();
    }

    // M = inv_kernel * fwd_gain^T  =>  (nSources x nChannels) * (nChannels x nSources)^T
    // Actually gain is (nChannels x nSources), so gain^T is (nSources x nChannels)
    // M = kernel * gain  =>  (nSources x nChannels) * (nChannels x nSources) = (nSources x nSources)
    MatrixXd M = kernel * gain;

    return M;
}
