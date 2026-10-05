//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_cortical_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref MNELIB::MNECorticalMap.
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

    if (!fwd.sol || fwd.sol->data.rows() == 0) {
        qWarning("MNECorticalMap::makeCorticalMap - Forward solution is empty.");
        return MatrixXd();
    }
    // The kernel's columns are the inverse operator's channels; pick those rows of the gain by name, as
    // mne-python's make_inverse_resolution_matrix does.
    MatrixXd gain = fwd.sol->data;
    const QStringList& invChannels = inv.eigen_fields ? inv.eigen_fields->col_names : QStringList();
    if (invChannels.size() == kernel.cols() && fwd.sol->row_names.size() == gain.rows()) {
        MatrixXd picked(kernel.cols(), gain.cols());
        for (int c = 0; c < invChannels.size(); ++c) {
            const int row = static_cast<int>(fwd.sol->row_names.indexOf(invChannels[c]));
            if (row < 0) {
                qWarning("MNECorticalMap::makeCorticalMap - Channel %s is not in the forward solution.",
                         invChannels[c].toUtf8().constData());
                return MatrixXd();
            }
            picked.row(c) = gain.row(row);
        }
        gain = picked;
    }

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

    return kernel * gain;
}
