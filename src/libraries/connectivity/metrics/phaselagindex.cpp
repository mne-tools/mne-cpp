//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     phaselagindex.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     January 2018
 * @brief    Implementation of @ref mne::PhaseLagIndex - the volume-conduction-robust phase-asymmetry estimator of Stam, Nolte & Daffertshofer (HBM 2007).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselagindex.h"
#include <base/log.h>

#include <vector>
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

PhaseLagIndex::PhaseLagIndex()
{
}

//*******************************************************************************************************

Network PhaseLagIndex::calculate(ConnectivitySettings& connectivitySettings)
{
    //    QElapsedTimer timer;
    //    qint64 iTime = 0;
    //    timer.start();

    Network finalNetwork("PLI");

    if (connectivitySettings.empty()) {
        Log::debug("PhaseLagIndex::calculate - Input data is empty");
        return finalNetwork;
    }

    if (!connectivitySettings.keeps_trial_data()) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    //Create nodes
    int iNRows = connectivitySettings.at(0).matData.rows();
    add_nodes(finalNetwork, iNRows, connectivitySettings.getNodePositions());

    // Check that iNfft >= signal length
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    std::pair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    const int binStart = connectivitySettings.frequency_bins().first;
    const int binAmount = connectivitySettings.frequency_bins().second;

    // Pass information about the FFT length. Use iNFreqs because we only use the half spectrum
    finalNetwork.set_fft_size(iNFreqs);
    finalNetwork.set_used_freq_bins(binAmount);

    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers,
                binStart,
                binAmount,
                connectivitySettings.keeps_trial_data());
    };

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Compute DSWPLV in parallel for all trials
    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
                                             computeLambda);
    result.waitForFinished();

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Compute PLI
    computePLI(connectivitySettings,
               finalNetwork);

    //    iTime = timer.elapsed();
    //    timer.restart();

    return finalNetwork;
}

//=============================================================================================================

void PhaseLagIndex::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                            std::vector<std::pair<int, MatrixXcd>>& vecPairCsdSum,
                            std::vector<std::pair<int, MatrixXd>>& vecPairCsdImagSignSum,
                            QMutex& mutex,
                            int iNRows,
                            int iNFreqs,
                            int iNfft,
                            const std::pair<MatrixXd, VectorXd>& tapers,
                            int binStart,
                            int binAmount,
                            bool keepTrialData)
{
    if (std::ssize(inputData.vecPairCsdImagSign) == iNRows) {
        return;
    }

    int i;

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if (inputData.vecTapSpectra.empty()) {
        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra
            for (int j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0, 0, 1, rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));
                    ;
                } else {
                    vecInputFFT = rowData.cwiseProduct(tapers.first.row(j));
                }

                // FFT for freq domain returning the half spectrum and multiply taper weights
                fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecTmpFreq * tapers.second(j);
            }

            inputData.vecTapSpectra.push_back(matTapSpectrum);
        }
    }

    // Compute CSD
    if (inputData.vecPairCsd.empty()) {
        MatrixXcd matCsd = MatrixXcd(iNRows, binAmount);

        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

        bool bNfftEven = false;
        if (iNfft % 2 == 0) {
            bNfftEven = true;
        }

        for (i = 0; i < iNRows; ++i) {
            for (int j = i; j < iNRows; ++j) {
                // Compute CSD (average over tapers if necessary)
                matCsd.row(j) = inputData.vecTapSpectra.at(i).block(0, binStart, inputData.vecTapSpectra.at(i).rows(), binAmount).cwiseProduct(inputData.vecTapSpectra.at(j).block(0, binStart, inputData.vecTapSpectra.at(j).rows(), binAmount).conjugate()).colwise().sum() / denomCSD;

                // Divide first and last element by 2 due to half spectrum
                if (binStart == 0) {
                    matCsd.row(j)(0) /= 2.0;
                }

                if (bNfftEven && binStart + binAmount >= iNFreqs) {
                    matCsd.row(j).tail(1) /= 2.0;
                }
            }

            inputData.vecPairCsd.push_back(std::pair<int, MatrixXcd>(i, matCsd));
            inputData.vecPairCsdImagSign.push_back(std::pair<int, MatrixXd>(i, matCsd.imag().cwiseSign()));
        }

        mutex.lock();

        if (vecPairCsdSum.empty()) {
            vecPairCsdSum = inputData.vecPairCsd;
            vecPairCsdImagSignSum = inputData.vecPairCsdImagSign;
        } else {
            for (int j = 0; j < std::ssize(vecPairCsdSum); ++j) {
                vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
                vecPairCsdImagSignSum[j].second += inputData.vecPairCsdImagSign.at(j).second;
            }
        }

        mutex.unlock();
    } else {
        if (inputData.vecPairCsdImagSign.empty()) {
            for (i = 0; i < std::ssize(inputData.vecPairCsd); ++i) {
                inputData.vecPairCsdImagSign.push_back(std::pair<int, MatrixXd>(i, inputData.vecPairCsd.at(i).second.imag().cwiseSign()));
            }

            mutex.lock();

            if (vecPairCsdImagSignSum.empty()) {
                vecPairCsdImagSignSum = inputData.vecPairCsdImagSign;
            } else {
                for (int j = 0; j < std::ssize(vecPairCsdImagSignSum); ++j) {
                    vecPairCsdImagSignSum[j].second += inputData.vecPairCsdImagSign.at(j).second;
                }
            }

            mutex.unlock();
        }
    }

    if (!keepTrialData) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
        inputData.vecPairCsdImagSign.clear();
    }
}

//=============================================================================================================

void PhaseLagIndex::computePLI(ConnectivitySettings& connectivitySettings,
                               Network& finalNetwork)
{
    // Compute final PLI and create Network
    MatrixXd matNom;
    MatrixXd matWeight;
    int j;

    for (int i = 0; i < std::ssize(connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum); ++i) {
        matNom = connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum.at(i).second.cwiseAbs() / connectivitySettings.size();

        for (j = i; j < matNom.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            finalNetwork.add_edge(NetworkEdge(i, j, std::move(matWeight)));
        }
    }
}
