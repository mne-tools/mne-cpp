//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     mne_cortical_map.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::MNECorticalMap.
 *
 * Implements binary read of the FreeSurfer @c .map file and the
 * construction of the per-vertex weight table.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_cortical_map.h"
#include <fiff/fiff_ch_name.h>
#include <base/log.h>
#include <base/string_utils.h>
#include "mne_forward_solution.h"
#include "mne_inverse_operator.h"

#include <fiff/fiff_info.h>
#include <fiff/fiff_named_matrix.h>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd MNECorticalMap::makeCorticalMap(
    const MNEForwardSolution& fwd,
    const MNEInverseOperator& inv,
    [[maybe_unused]] const FiffInfo& info)
{
    // Get the inverse kernel (nSources x nChannels)
    const MatrixXd& kernel = const_cast<MNEInverseOperator&>(inv).getKernel();
    if (kernel.rows() == 0 || kernel.cols() == 0) {
        mne::Log::warning("MNECorticalMap::makeCorticalMap - Inverse kernel is empty. Make sure the inverse operator has been prepared (assemble_kernel).");
        return MatrixXd();
    }

    if (fwd.sol.data.rows() == 0) {
        mne::Log::warning("MNECorticalMap::makeCorticalMap - Forward solution is empty.");
        return MatrixXd();
    }
    // A loose inverse kernel is in surface coordinates; rotate a Cartesian free forward to match
    // (mne-python _convert_forward_match_inv).
    MNEForwardSolution matched = fwd;
    const bool looseInverse = inv.orient_prior.data.size() > 0 && (inv.orient_prior.data.array() < 1.0).any();
    if (looseInverse && !matched.isFixedOrient())
        matched.convert_to_surf_ori();

    // The kernel's columns are the inverse operator's channels; pick those rows of the gain by name, as
    // mne-python's make_inverse_resolution_matrix does.
    MatrixXd gain = matched.sol.data;
    const StringList& invChannels = inv.eigen_fields.col_names;
    if (static_cast<int>(invChannels.size()) == kernel.cols() && static_cast<int>(matched.sol.row_names.size()) == gain.rows()) {
        MatrixXd picked(kernel.cols(), gain.cols());
        for (int c = 0; c < static_cast<int>(invChannels.size()); ++c) {
            const int row = static_cast<int>(mne::channel_index(matched.sol.row_names, invChannels[c]));
            if (row < 0) {
                mne::Log::warning("MNECorticalMap::makeCorticalMap - Channel {} is not in the forward solution.", invChannels[c]);
                return MatrixXd();
            }
            picked.row(c) = gain.row(row);
        }
        gain = picked;
    }

    // Verify dimension compatibility
    if (kernel.cols() != gain.rows()) {
        mne::Log::warning("MNECorticalMap::makeCorticalMap - Dimension mismatch: kernel is {} x {}, gain is {} x {}", kernel.rows(), kernel.cols(), gain.rows(), gain.cols());
        return MatrixXd();
    }

    return kernel * gain;
}
