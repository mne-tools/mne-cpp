//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     partial_directed_coherence.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref CONNLIB::PartialDirectedCoherence - column-normalised |A(f)| directional connectivity (Baccala & Sameshima 2001), direct paths only.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNLIB;
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

    if(connectivitySettings.isEmpty()) {
        qDebug() << "PartialDirectedCoherence::calculate - Input data is empty";
        return finalNetwork;
    }

    finalNetwork.setSamplingFrequency(connectivitySettings.getSamplingFrequency());

    // Average trial data for MVAR fitting
    const int nTrials = connectivitySettings.size();
    MatrixXd matDataAvg = connectivitySettings.at(0).matData;
    for(int t = 1; t < nTrials; ++t) {
        matDataAvg += connectivitySettings.at(t).matData;
    }
    matDataAvg /= static_cast<double>(nTrials);

    const int nCh = static_cast<int>(matDataAvg.rows());
    const int iNfft = connectivitySettings.getFFTSize();
    const int iNFreqs = static_cast<int>(std::floor(iNfft / 2.0)) + 1;

    finalNetwork.setFFTSize(iNFreqs);
    finalNetwork.setUsedFreqBins(iNFreqs);

    // Create nodes
    RowVectorXf rowVert = RowVectorXf::Zero(3);
    for(int i = 0; i < nCh; ++i) {
        rowVert = RowVectorXf::Zero(3);
        if(connectivitySettings.getNodePositions().rows() != 0 && i < connectivitySettings.getNodePositions().rows()) {
            rowVert(0) = connectivitySettings.getNodePositions().row(i)(0);
            rowVert(1) = connectivitySettings.getNodePositions().row(i)(1);
            rowVert(2) = connectivitySettings.getNodePositions().row(i)(2);
        }
        finalNetwork.append(NetworkNode::SPtr(new NetworkNode(i, rowVert)));
    }

    // Fit MVAR model
    MvarModel model;
    model.fit(matDataAvg);

    // Compute A(f) = I - sum_{k=1}^{p} A_k * exp(-2*pi*i*f*k) at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    QVector<MatrixXd> coeffs = model.coefficients();
    int p = model.order();

    const MatrixXcd matI = MatrixXcd::Identity(nCh, nCh);
    const std::complex<double> jImag(0.0, 1.0);

    // Compute PDC: PDC_{ij}(f) = |A_{ij}(f)| / sqrt(sum_k |A_{kj}(f)|^2)
    // (normalized per column j)
    for(int i = 0; i < nCh; ++i) {
        for(int j = 0; j < nCh; ++j) {
            MatrixXd matWeight(iNFreqs, 1);

            for(int fi = 0; fi < iNFreqs; ++fi) {
                // Compute A(f) at this frequency
                MatrixXcd matAf = matI;
                for(int k = 0; k < p; ++k) {
                    const double phase = -2.0 * M_PI * vecFreqs(fi) * (k + 1);
                    matAf -= coeffs[k].cast<std::complex<double>>() * std::exp(jImag * phase);
                }

                // Column normalization: sqrt(sum_k |A_{kj}(f)|^2)
                double colNorm = 0.0;
                for(int k = 0; k < nCh; ++k) {
                    colNorm += std::norm(matAf(k, j));
                }
                colNorm = std::sqrt(colNorm);

                if(colNorm > 0.0) {
                    matWeight(fi, 0) = std::abs(matAf(i, j)) / colNorm;
                } else {
                    matWeight(fi, 0) = 0.0;
                }
            }

            QSharedPointer<NetworkEdge> pEdge =
                QSharedPointer<NetworkEdge>(new NetworkEdge(j, i, matWeight));

            finalNetwork.getNodeAt(j)->append(pEdge);
            finalNetwork.getNodeAt(i)->append(pEdge);
            finalNetwork.append(pEdge);
        }
    }

    return finalNetwork;
}
