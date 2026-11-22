//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     granger_causality.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref mne::GrangerCausality - spectral Granger Causality (Geweke 1982 formulation) between every ordered channel pair, computed from the shared @ref mne::MvarModel fit.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "granger_causality.h"

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

GrangerCausality::GrangerCausality()
{
}

//=============================================================================================================

Network GrangerCausality::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("GC");

    if (connectivitySettings.empty()) {
        mne::Log::debug("GrangerCausality::calculate - Input data is empty");
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

    // Compute transfer function and spectral matrix at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    std::vector<MatrixXcd> vecH = model.transferFunction(vecFreqs);
    std::vector<MatrixXcd> vecS = model.spectralMatrix(vecFreqs);
    MatrixXd matSigma = model.noiseCov();

    // Compute spectral Granger causality for each directed pair
    // GC_{j->i}(f) = ln( S_{ii}(f) / (S_{ii}(f) - gamma_{ij} * |H_{ij}(f)|^2) )
    // where gamma_{ij} = Sigma_{jj} - Sigma_{ij}^2 / Sigma_{ii}
    for (int i = 0; i < nCh; ++i) {
        for (int j = 0; j < nCh; ++j) {
            if (i == j) {
                continue;
            }

            MatrixXd matWeight(iNFreqs, 1);

            const double gammaIJ = matSigma(j, j) - (matSigma(i, j) * matSigma(i, j)) / matSigma(i, i);

            for (int fi = 0; fi < iNFreqs; ++fi) {
                const double sII = vecS[fi](i, i).real();
                const double hIJ2 = std::norm(vecH[fi](i, j));
                const double denom = sII - gammaIJ * hIJ2;

                if (denom > 0.0 && sII > 0.0) {
                    matWeight(fi, 0) = std::log(sII / denom);
                } else {
                    matWeight(fi, 0) = 0.0;
                }
            }

            finalNetwork.add_edge(NetworkEdge(j, i, std::move(matWeight)));
        }
    }

    return finalNetwork;
}
