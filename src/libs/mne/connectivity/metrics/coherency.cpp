//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     coherency.cpp
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Implementation of @ref mne::Coherency - shared cross/auto-spectral core for magnitude-squared coherence and the volume-conduction-robust imaginary coherence of Nolte et al. (2004).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "coherency.h"

#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include <mne/base/thread_pool.h>
#include <mne/base/log.h>
#include <mne/math/spectral.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <vector>
#include <utility>

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

Coherency::Coherency()
{
}

//=============================================================================================================

void Coherency::calculateAbs(Network& finalNetwork,
                             ConnectivitySettings& connectivitySettings)
{
    if (connectivitySettings.empty()) {
        Log::debug("Coherency::calculateReal - Input data is empty");
        return;
    }

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    std::pair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;
    const int binStart = connectivitySettings.frequency_bins().first;
    const int binAmount = connectivitySettings.frequency_bins().second;

    // Compute PSD/CSD for each trial
    std::mutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().matPsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers,
                binStart,
                binAmount,
                connectivitySettings.keeps_trial_data());
    };
    auto& items = connectivitySettings.getTrialData();
    ThreadPool::global().parallel_for(0, items.size(), [&](std::size_t k) { computeLambda(items[k]); });
    // Compute CSD/sqrt(PSD_X * PSD_Y)
    const MatrixXd& matPsdSum = connectivitySettings.getIntermediateSumData().matPsdSum;
    const auto rows = ThreadPool::global().parallel_map(connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                                                        [&matPsdSum](const std::pair<int, MatrixXcd>& pairInput) { return rowEdges(pairInput, matPsdSum, false); });
    for (const std::vector<NetworkEdge>& row : rows) {
        for (const NetworkEdge& edge : row) {
            finalNetwork.add_edge(edge);
        }
    }
}

//=============================================================================================================

void Coherency::calculateImag(Network& finalNetwork,
                              ConnectivitySettings& connectivitySettings)
{
    if (connectivitySettings.empty()) {
        Log::debug("Coherency::calculateImag - Input data is empty");
        return;
    }

#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    int iSignalLength = connectivitySettings.at(0).matData.cols();
    int iNfft = connectivitySettings.getFFTSize();

    // Generate tapers
    std::pair<MatrixXd, VectorXd> tapers = Spectral::generateTapers(iSignalLength, connectivitySettings.getWindowType());

    // Initialize vecPsdAvg and vecCsdAvg
    int iNRows = connectivitySettings.at(0).matData.rows();
    int iNFreqs = int(floor(iNfft / 2.0)) + 1;
    const int binStart = connectivitySettings.frequency_bins().first;
    const int binAmount = connectivitySettings.frequency_bins().second;

    // Compute PSD/CSD for each trial
    std::mutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().matPsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers,
                binStart,
                binAmount,
                connectivitySettings.keeps_trial_data());
    };
    auto& items = connectivitySettings.getTrialData();
    ThreadPool::global().parallel_for(0, items.size(), [&](std::size_t k) { computeLambda(items[k]); });
    // Compute CSD/sqrt(PSD_X * PSD_Y)
    const MatrixXd& matPsdSum = connectivitySettings.getIntermediateSumData().matPsdSum;
    const auto rows = ThreadPool::global().parallel_map(connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                                                        [&matPsdSum](const std::pair<int, MatrixXcd>& pairInput) { return rowEdges(pairInput, matPsdSum, true); });
    for (const std::vector<NetworkEdge>& row : rows) {
        for (const NetworkEdge& edge : row) {
            finalNetwork.add_edge(edge);
        }
    }
}

//=============================================================================================================

void Coherency::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                        MatrixXd& matPsdSum,
                        std::vector<std::pair<int, MatrixXcd>>& vecPairCsdSum,
                        std::mutex& mutex,
                        int iNRows,
                        int iNFreqs,
                        int iNfft,
                        const std::pair<MatrixXd, VectorXd>& tapers,
                        int binStart,
                        int binAmount,
                        bool keepTrialData)
{
    // A phase-lag metric may have computed the cross-spectra of the trial already, but not its spectra
    const bool hasPsd = inputData.matPsd.rows() == iNRows && inputData.matPsd.cols() == binAmount;
    const bool hasCsd = std::ssize(inputData.vecPairCsd) == iNRows;
    if (hasPsd && hasCsd) {
        return;
    }

    // Substract mean, compute tapered spectra and PSD
    // This code was copied and changed modified Utils/Spectra since we do not want to call the function due to time loss.
    bool bNfftEven = false;
    if (iNfft % 2 == 0) {
        bNfftEven = true;
    }

    int i, j;

    if (std::ssize(inputData.vecTapSpectra) != iNRows) {
        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);
        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;
        MatrixXcd matTapSpectrum(tapers.first.rows(), iNFreqs);

        inputData.vecTapSpectra.clear();
        for (i = 0; i < iNRows; ++i) {
            // Substract mean
            rowData.array() = inputData.matData.row(i).array() - inputData.matData.row(i).mean();

            for (j = 0; j < tapers.first.rows(); j++) {
                // Zero padd if necessary. The zero padding in Eigen's FFT is only working for column vectors.
                if (rowData.cols() < iNfft) {
                    vecInputFFT.setZero(iNfft);
                    vecInputFFT.block(0, 0, 1, rowData.cols()) = rowData.cwiseProduct(tapers.first.row(j));
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

    if (!hasPsd) {
        const double denomPSD = tapers.second.cwiseAbs2().sum() / 2.0;
        inputData.matPsd = MatrixXd(iNRows, binAmount);

        for (i = 0; i < iNRows; ++i) {
            // Compute PSD (average over tapers if necessary).
            inputData.matPsd.row(i) = inputData.vecTapSpectra.at(i).block(0, binStart, inputData.vecTapSpectra.at(i).rows(), binAmount).cwiseAbs2().colwise().sum() / denomPSD;

            // Divide first and last element by 2 due to half spectrum
            if (binStart == 0) {
                inputData.matPsd.row(i)(0) /= 2.0;
            }

            if (bNfftEven && binStart + binAmount >= iNFreqs) {
                inputData.matPsd.row(i).tail(1) /= 2.0;
            }
        }

        {
            const std::lock_guard lock(mutex);

            if (matPsdSum.rows() == 0 || matPsdSum.cols() == 0) {
                matPsdSum = inputData.matPsd;
            } else {
                matPsdSum += inputData.matPsd;
            }
        }
    }
    // Compute CSD
    if (!hasCsd) {
        inputData.vecPairCsd.clear();

        //MatrixXcd matCsd = MatrixXcd(iNRows, iNFreqs);
        MatrixXcd matCsd = MatrixXcd(iNRows, binAmount);

        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

        for (i = 0; i < iNRows; ++i) {
            for (j = i; j < iNRows; ++j) {
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
        }

        {
            const std::lock_guard lock(mutex);

            if (vecPairCsdSum.empty()) {
                vecPairCsdSum = inputData.vecPairCsd;
            } else {
                for (j = 0; j < std::ssize(vecPairCsdSum); ++j) {
                    vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
                }
            }
        }
    }
    //Do not store data to save memory
    if (!keepTrialData) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
    }
}

//=============================================================================================================

std::vector<NetworkEdge> Coherency::rowEdges(const std::pair<int, MatrixXcd>& pairInput,
                                             const MatrixXd& matPsdSum,
                                             bool imaginary)
{
    MatrixXd matPSDtmp(matPsdSum.rows(), matPsdSum.cols());
    RowVectorXd rowPsdSum = matPsdSum.row(pairInput.first);

    for (int j = 0; j < matPSDtmp.rows(); ++j) {
        matPSDtmp.row(j) = rowPsdSum.cwiseProduct(matPsdSum.row(j));
    }

    // Average. Note that the number of trials cancel each other out.
    const MatrixXcd matCohy = pairInput.second.cwiseQuotient(matPSDtmp.cwiseSqrt());

    const int i = pairInput.first;
    std::vector<NetworkEdge> edges;
    edges.reserve(static_cast<std::size_t>(std::max<Eigen::Index>(matCohy.rows() - i, 0)));
    for (int j = i; j < matCohy.rows(); ++j) {
        MatrixXd matWeight = imaginary ? MatrixXd(matCohy.row(j).imag().transpose()) : MatrixXd(matCohy.row(j).cwiseAbs().transpose());
        edges.emplace_back(i, j, std::move(matWeight));
    }
    return edges;
}
