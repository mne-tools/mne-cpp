//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     correlation.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of @ref mne::Correlation - trial-averaged zero-lag Pearson correlation between every channel pair, run in parallel via @c QtConcurrent::mappedReduced.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "correlation.h"
#include <base/log.h>
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
#include <QElapsedTimer>
#include <QtConcurrent>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

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

Correlation::Correlation()
{
}

//=============================================================================================================

Network Correlation::calculate(ConnectivitySettings& connectivitySettings)
{
    //    QElapsedTimer timer;
    //    qint64 iTime = 0;
    //    timer.start();

    Network finalNetwork("COR");

    if (connectivitySettings.empty()) {
        mne::Log::debug("Correlation::calculate - Input data is empty");
        return finalNetwork;
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    add_nodes(finalNetwork, rows, connectivitySettings.getNodePositions());

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Calculate connectivity matrix over epochs and average afterwards
    //    double dScalingStep = 1.0/matDataList.size();
    //    dataTemp.matInputData = dScalingStep * (i+1) * matDataList.at(i);

    QFuture<MatrixXd> resultMat = QtConcurrent::mappedReduced(connectivitySettings.getTrialData(),
                                                              compute,
                                                              reduce);
    resultMat.waitForFinished();

    MatrixXd matDist = resultMat.result() / connectivitySettings.size();

    //    MatrixXd matDist;

    //    for(int i = 0; i < connectivitySettings.getTrialData().size(); ++i) {
    //        reduce(matDist, compute(connectivitySettings.getTrialData().at(i)));
    //    }

    //    matDist /= connectivitySettings.size();

    //    iTime = timer.elapsed();
    //    timer.restart();

    //Add edges to network
    int j;

    for (int i = 0; i < matDist.rows(); ++i) {
        for (j = i; j < matDist.cols(); ++j) {
            finalNetwork.add_edge(NetworkEdge(i, j, MatrixXd::Constant(1, 1, matDist(i, j))));
        }
    }

    //    iTime = timer.elapsed();
    //    timer.restart();

    return finalNetwork;
}

//=============================================================================================================

MatrixXd Correlation::compute(const ConnectivitySettings::IntermediateTrialData& inputData)
{
    const MatrixXd centered = inputData.matData.colwise() - inputData.matData.rowwise().mean();
    const VectorXd norms = centered.rowwise().norm();
    MatrixXd matDist = centered * centered.transpose();

    for (int i = 0; i < matDist.rows(); ++i) {
        for (int j = 0; j < matDist.cols(); ++j) {
            const double denom = norms(i) * norms(j);
            matDist(i, j) = denom > 0.0 ? matDist(i, j) / denom : 0.0;
        }
    }

    return matDist;
}

//=============================================================================================================

void Correlation::reduce(MatrixXd& resultData,
                         const MatrixXd& data)
{
    if (resultData.rows() != data.rows() || resultData.cols() != data.cols()) {
        resultData.resize(data.rows(), data.cols());
        resultData.setZero();
    }

    resultData += data;
}
