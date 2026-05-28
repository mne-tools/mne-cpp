//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file granger_causality.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of @ref CONNLIB::GrangerCausality - spectral Granger Causality (Geweke 1982 formulation) between every ordered channel pair, computed from the shared @ref CONNLIB::MvarModel fit.
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

GrangerCausality::GrangerCausality()
{
}

//=============================================================================================================

Network GrangerCausality::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("GC");

    if(connectivitySettings.isEmpty()) {
        qDebug() << "GrangerCausality::calculate - Input data is empty";
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

    // Compute transfer function and spectral matrix at normalized frequencies
    VectorXd vecFreqs = VectorXd::LinSpaced(iNFreqs, 0.0, 0.5);
    QVector<MatrixXcd> vecH = model.transferFunction(vecFreqs);
    QVector<MatrixXcd> vecS = model.spectralMatrix(vecFreqs);
    MatrixXd matSigma = model.noiseCov();

    // Compute spectral Granger causality for each directed pair
    // GC_{j->i}(f) = ln( S_{ii}(f) / (S_{ii}(f) - gamma_{ij} * |H_{ij}(f)|^2) )
    // where gamma_{ij} = Sigma_{jj} - Sigma_{ij}^2 / Sigma_{ii}
    for(int i = 0; i < nCh; ++i) {
        for(int j = 0; j < nCh; ++j) {
            if(i == j) {
                continue;
            }

            MatrixXd matWeight(iNFreqs, 1);

            const double gammaIJ = matSigma(j, j) - (matSigma(i, j) * matSigma(i, j)) / matSigma(i, i);

            for(int fi = 0; fi < iNFreqs; ++fi) {
                const double sII = vecS[fi](i, i).real();
                const double hIJ2 = std::norm(vecH[fi](i, j));
                const double denom = sII - gammaIJ * hIJ2;

                if(denom > 0.0 && sII > 0.0) {
                    matWeight(fi, 0) = std::log(sII / denom);
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
