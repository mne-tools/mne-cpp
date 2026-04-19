//=============================================================================================================
/**
 * @file     partial_directed_coherence.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
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
 * @brief    PartialDirectedCoherence class definition.
 *
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
