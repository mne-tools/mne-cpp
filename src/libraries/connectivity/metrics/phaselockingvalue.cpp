//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2018-2026 MNE-CPP Authors
 *
 * @file     phaselockingvalue.cpp
 * @author   Daniel Strohmeier <daniel.strohmeier@gmail.com>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     April 2018
 * @brief    Implementation of @ref mne::PhaseLockingValue - resultant length of the unit-modulus cross-spectrum (Lachaux et al. HBM 1999).
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "phaselockingvalue.h"

#include "../network/networknode.h"
#include "../network/networkedge.h"
#include "../network/network.h"
#include <base/thread_pool.h>
#include <base/log.h>
#include <math/spectral.h>

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

PhaseLockingValue::PhaseLockingValue()
{
}

//*******************************************************************************************************

Network PhaseLockingValue::calculate(ConnectivitySettings& connectivitySettings)
{
    Network finalNetwork("PLV");

    if (connectivitySettings.empty()) {
        Log::debug("PhaseLockingValue::calculate - Input data is empty");
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

    std::mutex mutex;

    std::function<void(ConnectivitySettings::IntermediateTrialData&)> computeLambda = [&](ConnectivitySettings::IntermediateTrialData& inputData) {
        compute(inputData,
                connectivitySettings.getIntermediateSumData().vecPairCsdSum,
                connectivitySettings.getIntermediateSumData().vecPairCsdNormalizedSum,
                mutex,
                iNRows,
                iNFreqs,
                iNfft,
                tapers,
                binStart,
                binAmount,
                connectivitySettings.keeps_trial_data());
    };
    // Compute PLV in parallel for all trials
    auto& items = connectivitySettings.getTrialData();
    ThreadPool::global().parallel_for(0, items.size(), [&](std::size_t k) { computeLambda(items[k]); });
    // Compute PLV
    computePLV(connectivitySettings,
               finalNetwork);
    return finalNetwork;
}

//=============================================================================================================

void PhaseLockingValue::compute(ConnectivitySettings::IntermediateTrialData& inputData,
                                std::vector<std::pair<int, Eigen::MatrixXcd>>& vecPairCsdSum,
                                std::vector<std::pair<int, MatrixXcd>>& vecPairCsdNormalizedSum,
                                std::mutex& mutex,
                                int iNRows,
                                int iNFreqs,
                                int iNfft,
                                const std::pair<MatrixXd, VectorXd>& tapers,
                                int binStart,
                                int binAmount,
                                bool keepTrialData)
{
    if (std::ssize(inputData.vecPairCsdNormalized) == iNRows) {
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
        MatrixXcd matCsd = MatrixXcd(iNRows, binAmount);

        bool bNfftEven = false;
        if (iNfft % 2 == 0) {
            bNfftEven = true;
        }

        double denomCSD = sqrt(tapers.second.cwiseAbs2().sum()) * sqrt(tapers.second.cwiseAbs2().sum()) / 2.0;

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
            inputData.vecPairCsdNormalized.push_back(std::pair<int, MatrixXcd>(i, matCsd.cwiseQuotient(matCsd.cwiseAbs())));
        }

        {
            const std::lock_guard lock(mutex);

            if (vecPairCsdSum.empty()) {
                vecPairCsdSum = inputData.vecPairCsd;
                vecPairCsdNormalizedSum = inputData.vecPairCsdNormalized;
            } else {
                for (int j = 0; j < std::ssize(vecPairCsdSum); ++j) {
                    vecPairCsdSum[j].second += inputData.vecPairCsd.at(j).second;
                    vecPairCsdNormalizedSum[j].second += inputData.vecPairCsdNormalized.at(j).second;
                }
            }
        }
    } else {
        if (inputData.vecPairCsdNormalized.empty()) {
            for (i = 0; i < iNRows; ++i) {
                inputData.vecPairCsdNormalized.push_back(std::pair<int, MatrixXcd>(i, inputData.vecPairCsd.at(i).second.cwiseQuotient(inputData.vecPairCsd.at(i).second.cwiseAbs())));
            }

            {
                const std::lock_guard lock(mutex);

                if (vecPairCsdNormalizedSum.empty()) {
                    vecPairCsdNormalizedSum = inputData.vecPairCsdNormalized;
                } else {
                    for (int j = 0; j < std::ssize(vecPairCsdNormalizedSum); ++j) {
                        vecPairCsdNormalizedSum[j].second += inputData.vecPairCsdNormalized.at(j).second;
                    }
                }
            }
        }
    }

    if (!keepTrialData) {
        inputData.vecPairCsd.clear();
        inputData.vecTapSpectra.clear();
        inputData.vecPairCsdNormalized.clear();
    }
}

//=============================================================================================================

void PhaseLockingValue::computePLV(ConnectivitySettings& connectivitySettings,
                                   Network& finalNetwork)
{
    // Compute final PLV and create Network
    MatrixXd matNom;
    MatrixXd matWeight;
    int j;

    for (int i = 0; i < connectivitySettings.at(0).matData.rows(); ++i) {
        matNom = connectivitySettings.getIntermediateSumData().vecPairCsdNormalizedSum.at(i).second.cwiseAbs() / connectivitySettings.size();

        for (j = i; j < connectivitySettings.at(0).matData.rows(); ++j) {
            matWeight = matNom.row(j).transpose();

            finalNetwork.add_edge(NetworkEdge(i, j, std::move(matWeight)));
        }
    }
}
