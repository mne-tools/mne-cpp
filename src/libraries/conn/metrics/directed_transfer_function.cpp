//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     directed_transfer_function.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April 2026
 * @brief    Implementation of @ref CONNLIB::DirectedTransferFunction - row-normalised |H(f)|^2 directional connectivity (Kaminski & Blinowska 1991), driven by the shared @ref CONNLIB::MvarModel fit.
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

DirectedTransferFunction::DirectedTransferFunction()
{
}

//=============================================================================================================

Network DirectedTransferFunction::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("DTF");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "DirectedTransferFunction::calculate - Input data is empty";
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

    // Compute transfer function at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    QVector<MatrixXcd> vecH = model.transferFunction(vecFreqs);

    // Compute DTF: DTF_{ij}(f) = |H_{ij}(f)|^2 / sum_k |H_{ik}(f)|^2
    // (normalized per row i)
    for(int i = 0; i < nCh; ++i) {
        for(int j = 0; j < nCh; ++j) {
            MatrixXd matWeight(iNFreqs, 1);

            for(int fi = 0; fi < iNFreqs; ++fi) {
                // Row normalization: sum of |H_{ik}(f)|^2 over all k
                double rowNorm = 0.0;
                for(int k = 0; k < nCh; ++k) {
                    rowNorm += std::norm(vecH[fi](i, k));
                }

                if(rowNorm > 0.0) {
                    matWeight(fi, 0) = std::norm(vecH[fi](i, j)) / rowNorm;
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
