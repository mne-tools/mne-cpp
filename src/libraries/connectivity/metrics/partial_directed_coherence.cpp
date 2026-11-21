//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     partial_directed_coherence.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::PartialDirectedCoherence - column-normalised |A(f)| directional connectivity (Baccala & Sameshima 2001), direct paths only.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "partial_directed_coherence.h"

#include "mvar_model.h"
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include "../connectivitysettings.h"
#include <base/log.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <cmath>
#include <numbers>

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

PartialDirectedCoherence::PartialDirectedCoherence()
{
}

//=============================================================================================================

Network PartialDirectedCoherence::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("PDC");

    if (connectivitySettings.empty()) {
        mne::Log::debug("PartialDirectedCoherence::calculate - Input data is empty");
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

    // Compute A(f) = I - sum_{k=1}^{p} A_k * exp(-2*pi*i*f*k) at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    std::vector<MatrixXd> coeffs = model.coefficients();
    int p = model.order();

    const MatrixXcd matI = MatrixXcd::Identity(nCh, nCh);
    const std::complex<double> jImag(0.0, 1.0);

    // Compute PDC: PDC_{ij}(f) = |A_{ij}(f)| / sqrt(sum_k |A_{kj}(f)|^2)
    // (normalized per column j)
    for (int i = 0; i < nCh; ++i) {
        for (int j = 0; j < nCh; ++j) {
            MatrixXd matWeight(iNFreqs, 1);

            for (int fi = 0; fi < iNFreqs; ++fi) {
                // Compute A(f) at this frequency
                MatrixXcd matAf = matI;
                for (int k = 0; k < p; ++k) {
                    const double phase = -2.0 * std::numbers::pi * vecFreqs(fi) * (k + 1);
                    matAf -= coeffs[k].cast<std::complex<double>>() * std::exp(jImag * phase);
                }

                // Column normalization: sqrt(sum_k |A_{kj}(f)|^2)
                double colNorm = 0.0;
                for (int k = 0; k < nCh; ++k) {
                    colNorm += std::norm(matAf(k, j));
                }
                colNorm = std::sqrt(colNorm);

                if (colNorm > 0.0) {
                    matWeight(fi, 0) = std::abs(matAf(i, j)) / colNorm;
                } else {
                    matWeight(fi, 0) = 0.0;
                }
            }

            finalNetwork.add_edge(NetworkEdge(j, i, std::move(matWeight)));
        }
    }

    return finalNetwork;
}
