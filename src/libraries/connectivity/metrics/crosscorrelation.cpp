//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     crosscorrelation.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of @ref mne::CrossCorrelation - lag-resolved cross-correlation between every channel pair, computed via the inverse FFT of the DPSS-tapered cross-spectrum.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "crosscorrelation.h"
#include <base/log.h>

#include <utility>
#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"

#include <math/spectral.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>
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

CrossCorrelation::CrossCorrelation()
{
}

//=============================================================================================================

Network CrossCorrelation::calculate(ConnectivitySettings& connectivitySettings)
{
    //    QElapsedTimer timer;
    //    qint64 iTime = 0;
    //    timer.start();

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    Network finalNetwork("XCOR");

    if (connectivitySettings.empty()) {
        Log::debug("CrossCorrelation::calculate - Input data is empty");
        return finalNetwork;
    }

    if (!connectivitySettings.keeps_trial_data()) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    add_nodes(finalNetwork, rows, connectivitySettings.getNodePositions());

    // Generate tapers
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    std::pair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Compute the cross correlation in parallel
    QMutex mutex;
    MatrixXd matDist;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                matDist,
                mutex,
                iNfft,
                tapers,
                connectivitySettings.keeps_trial_data());
    };

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Calculate connectivity matrix over epochs and average afterwards
    QFuture<void> resultMat = QtConcurrent::map(connectivitySettings.getTrialData(),
                                                computeLambda);
    resultMat.waitForFinished();

    matDist /= connectivitySettings.size();

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

void CrossCorrelation::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                               MatrixXd& matDist,
                               QMutex& mutex,
                               int iNfft,
                               const std::pair<MatrixXd, VectorXd>& tapers,
                               bool keepTrialData)
{
    //    QElapsedTimer timer;
    //    qint64 iTime = 0;
    //    timer.start();

    // Calculate tapered spectra if not available already
    RowVectorXd vecInputFFT, rowData;
    RowVectorXcd vecResultFreq;

    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    int i, j;
    int iNRows = inputData.matData.rows();

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if (inputData.vecTapSpectra.empty()) {
        int iNFreqs = int(floor(iNfft / 2.0)) + 1;
        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra
            for (j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0, 0, 1, rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));
                    ;
                } else {
                    vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                }

                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecResultFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecResultFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.push_back(matTapSpectrum);
        }
    }

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Perform multiplication and transform back to time domain to find max XCOR coefficient
    // Note that the result in time domain is mirrored around the center of the data (compared to Matlab)
    MatrixXd matDistTrial = MatrixXd::Zero(iNRows, iNRows);
    RowVectorXcd vecResultXCor;
    int idx = 0;
    double denom = tapers.second.sum();

    for (i = 0; i < std::ssize(inputData.vecTapSpectra); ++i) {
        vecResultFreq = inputData.vecTapSpectra.at(i).colwise().sum() / denom;

        for (j = i; j < std::ssize(inputData.vecTapSpectra); ++j) {
            vecResultXCor = vecResultFreq.cwiseProduct(inputData.vecTapSpectra.at(j).colwise().sum() / denom);

            fft.inv(vecInputFFT, vecResultXCor, iNfft);

            vecInputFFT.maxCoeff(&idx);

            matDistTrial(i, j) = vecInputFFT(idx);
        }
    }

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Sum up weights
    mutex.lock();

    if (matDist.rows() != matDistTrial.rows() || matDist.cols() != matDistTrial.cols()) {
        matDist.resize(matDistTrial.rows(), matDistTrial.cols());
        matDist.setZero();
    }

    matDist += matDistTrial;

    mutex.unlock();

    //    iTime = timer.elapsed();
    //    timer.restart();

    if (!keepTrialData) {
        inputData.vecTapSpectra.clear();
    }
}
