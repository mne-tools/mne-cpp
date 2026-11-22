//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     directed_transfer_function.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::DirectedTransferFunction - row-normalised |H(f)|^2 directional connectivity (Kaminski & Blinowska 1991), driven by the shared @ref mne::MvarModel fit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "directed_transfer_function.h"

#include "mvar_model.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include "../connectivitysettings.h"
#include <mne/base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

DirectedTransferFunction::DirectedTransferFunction()
{
}

//=============================================================================================================

Network DirectedTransferFunction::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("DTF");

    if (connectivitySettings.empty()) {
        mne::Log::debug("DirectedTransferFunction::calculate - Input data is empty");
        return finalNetwork;
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

    // Average trial data for MVAR fitting
    const int nTrials = connectivitySettings.size();
    MatrixXd matDataAvg = connectivitySettings.at(0).matData;
    for (int t = 1; t < nTrials; ++t) {
        matDataAvg += connectivitySettings.at(t).matData;
    }
    matDataAvg /= static_cast<double>(nTrials);

    const int nCh = static_cast<int>(matDataAvg.rows());
    const int iNfft = connectivitySettings.getFFTSize();
    const int iNFreqs = static_cast<int>(std::floor(iNfft / 2.0)) + 1;

    finalNetwork.set_fft_size(iNFreqs);
    finalNetwork.set_used_freq_bins(iNFreqs);

    // Create nodes
    add_nodes(finalNetwork, nCh, connectivitySettings.getNodePositions());

    // Fit MVAR model
    MvarModel model;
    model.fit(matDataAvg);

    // Compute transfer function at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    std::vector<MatrixXcd> vecH = model.transferFunction(vecFreqs);

    // Compute DTF: DTF_{ij}(f) = |H_{ij}(f)|^2 / sum_k |H_{ik}(f)|^2
    // (normalized per row i)
    for (int i = 0; i < nCh; ++i) {
        for (int j = 0; j < nCh; ++j) {
            MatrixXd matWeight(iNFreqs, 1);

            for (int fi = 0; fi < iNFreqs; ++fi) {
                // Row normalization: sum of |H_{ik}(f)|^2 over all k
                double rowNorm = 0.0;
                for (int k = 0; k < nCh; ++k) {
                    rowNorm += std::norm(vecH[fi](i, k));
                }

                if (rowNorm > 0.0) {
                    matWeight(fi, 0) = std::norm(vecH[fi](i, j)) / rowNorm;
                } else {
                    matWeight(fi, 0) = 0.0;
                }
            }

            finalNetwork.add_edge(NetworkEdge(j, i, std::move(matWeight)));
        }
    }

    return finalNetwork;
}
