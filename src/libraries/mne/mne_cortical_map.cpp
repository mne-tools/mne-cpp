//=============================================================================================================
/**
 * @file     mne_cortical_map.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.2.0
 * @date     April, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the MNECorticalMap Class.
 *
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
