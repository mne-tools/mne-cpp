//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     unbiasedsquaredphaselagindex.cpp
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Implementation of @ref mne::UnbiasedSquaredPhaseLagIndex - the small-sample-bias-corrected estimator of squared PLI.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "unbiasedsquaredphaselagindex.h"
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

UnbiasedSquaredPhaseLagIndex::UnbiasedSquaredPhaseLagIndex()
{
}

//*******************************************************************************************************

Network UnbiasedSquaredPhaseLagIndex::calculate(ConnectivitySettings& connectivitySettings)
{
    //    QElapsedTimer timer;
    //    qint64 iTime = 0;
    //    timer.start();

    Network finalNetwork("USPLI");

    if (connectivitySettings.empty()) {
        Log::debug("UnbiasedSquaredPhaseLagIndex::calculate - Input data is empty");
        return finalNetwork;
    }

    if (AbstractMetric::m_bStorageModeIsActive == false) {
        connectivitySettings.clearIntermediateData();
    }

    finalNetwork.set_sampling_frequency(connectivitySettings.getSamplingFrequency());

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    //Create nodes
    int rows = connectivitySettings.at(0).matData.rows();
    add_nodes(finalNetwork, rows, connectivitySettings.getNodePositions());

    // Check that iNfft >= signal length
    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    std::pair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;

    // Check if start and bin amount need to be reset to full spectrum
    if (m_iNumberBinStart == -1 ||
        m_iNumberBinAmount == -1 ||
        m_iNumberBinStart > iNFreqs ||
        m_iNumberBinAmount > iNFreqs ||
        m_iNumberBinAmount + m_iNumberBinStart > iNFreqs) {
        Log::debug("UnbiasedSquaredPhaseLagIndex::calculate - Resetting to full spectrum");
        AbstractMetric::m_iNumberBinStart = 0;
        AbstractMetric::m_iNumberBinAmount = iNFreqs;
    }

    // Pass information about the FFT length. Use iNFreqs because we only use the half spectrum
    finalNetwork.set_fft_size(iNFreqs);
    finalNetwork.set_used_freq_bins(AbstractMetric::m_iNumberBinAmount);

    QMutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers);
    };

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Compute DSWPLV in parallel for all trials
    QFuture<void> result = QtConcurrent::map(connectivitySettings.getTrialData(),
                                             computeLambda);
    result.waitForFinished();

    //    iTime = timer.elapsed();
    //    timer.restart();

    // Compute USPLI
    computeUSPLI(connectivitySettings,
                 finalNetwork);

    //    iTime = timer.elapsed();
    //    timer.restart();

    return finalNetwork;
}

//=============================================================================================================

void UnbiasedSquaredPhaseLagIndex::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                                           std::vector<std::pair<int, MatrixXcd>>& vecPairCsdSum,
                                           std::vector<std::pair<int, MatrixXd>>& vecPairCsdImagSignSum,
                                           QMutex& mutex,
                                           int iNRows,
                                           int iNFreqs,
                                           int iNfft,
                                           const std::pair<MatrixXd, VectorXd>& tapers)
{
    if (std::ssize(inputData.vecPairCsdImagSign) == iNRows) {
        return;
    }

    int i;

    // Calculate tapered spectra if not available already
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    if (std::ssize(inputData.vecTapSpectra) != iNRows) {
        inputData.vecTapSpectra.clear();

        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            // Calculate tapered spectra if not available already
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
        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

        bool bNfftEven = false;
        if (iNfft % 2 == 0) {
            bNfftEven = true;
        }

        MatrixXcd matCsd = MatrixXcd(iNRows, m_iNumberBinAmount);

        for (i = 0; i < iNRows; ++i) {
            for (int j = i; j < iNRows; ++j) {
                // Compute CSD (average over tapers if necessary)
                matCsd.row(j) = inputData.vecTapSpectra.at(i).block(0, m_iNumberBinStart, inputData.vecTapSpectra.at(i).rows(), m_iNumberBinAmount).cwiseProduct(inputData.vecTapSpectra.at(j).block(0, m_iNumberBinStart, inputData.vecTapSpectra.at(j).rows(), m_iNumberBinAmount).conjugate()).colwise().sum() / denomCSD;

                // Divide first and last element by 2 due to half spectrum
                if (m_iNumberBinStart == 0) {
                    matCsd.row(j)(0) /= 2.0;
                }

                if (bNfftEven && m_iNumberBinStart + m_iNumberBinAmount >= iNFreqs) {
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

    if (!m_bStorageModeIsActive) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
        inputData.vecPairCsdImagSign.clear();
    }
}

//=============================================================================================================

void UnbiasedSquaredPhaseLagIndex::computeUSPLI(ConnectivitySettings& connectivitySettings,
                                                Network& finalNetwork)
{
    // Compute final DSWPLV and create Network
    MatrixXd matNom;
    MatrixXd matWeight;
    int j;
    double dNTrials = double(connectivitySettings.size() - 1.0);

    for (int i = 0; i < std::ssize(connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum); ++i) {
        matNom = connectivitySettings.getIntermediateSumData().vecPairCsdImagSignSum.at(i).second.cwiseAbs() / connectivitySettings.size();
        matNom = (connectivitySettings.size() * matNom.array().square() - 1.0) / dNTrials;

        for (j = i; j < matNom.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            finalNetwork.add_edge(NetworkEdge(i, j, std::move(matWeight)));
        }
    }
}
