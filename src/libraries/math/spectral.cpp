//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     spectral.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    FFT, multi-taper combination and PSD/CSD assembly kernels.
 *
 * Implements the static estimators declared in @ref spectral.h. Each
 * tapered FFT runs through @c Eigen::FFT (Kiss-FFT backend) at
 * @c O(N log N); the @c K taper streams are then averaged into a PSD
 * with the one-sided scaling convention or combined into a complex
 * cross-spectral-density matrix per frequency bin. The matrix entry
 * point uses @c std::async to spread channels across worker
 * threads while keeping a deterministic ordering in the output vector.
 *
 * Numerical conventions match MNE-Python's @c psd_array_multitaper and
 * @c csd_array_multitaper so coherence, phase-locking and DICS results
 * computed downstream by mne_connectivity remain comparable across the
 * two stacks.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "spectral.h"
#include "math.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <unsupported/Eigen/FFT>

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <algorithm>
#include <cmath>
#include <future>
#include <thread>
#include <vector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace mne;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXcd Spectral::computeTaperedSpectraRow(const RowVectorXd& vecData,
                                             const MatrixXd& matTaper,
                                             int iNfft)
{
    FFT<double> fft;
    fft.SetFlag(fft.HalfSpectrum);

    //Check inputs
    if (vecData.cols() != matTaper.cols() || iNfft < vecData.cols()) {
        return MatrixXcd();
    }

    //FFT for freq domain returning the half spectrum
    RowVectorXd vecInputFFT;
    RowVectorXcd vecTmpFreq;
    MatrixXcd matTapSpectrum(matTaper.rows(), int(floor(iNfft / 2.0)) + 1);
    for (int i = 0; i < matTaper.rows(); i++) {
        vecInputFFT = vecData.cwiseProduct(matTaper.row(i));
        fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
        matTapSpectrum.row(i) = vecTmpFreq;
    }

    return matTapSpectrum;
}

//=============================================================================================================

std::vector<MatrixXcd> Spectral::computeTaperedSpectraMatrix(const MatrixXd& matData,
                                                             const MatrixXd& matTaper,
                                                             int iNfft,
                                                             bool bUseThreads)
{
#ifdef EIGEN_FFTW_DEFAULT
    fftw_make_planner_thread_safe();
#endif

    std::vector<MatrixXcd> finalResult;

    if (!bUseThreads) {
        // Sequential

        FFT<double> fft;
        fft.SetFlag(fft.HalfSpectrum);

        RowVectorXd vecInputFFT, rowData;
        RowVectorXcd vecTmpFreq;

        MatrixXcd matTapSpectrum(matTaper.rows(), int(floor(iNfft / 2.0)) + 1);
        int j;
        for (int i = 0; i < matData.rows(); ++i) {
            rowData = matData.row(i);

            //FFT for freq domain returning the half spectrum
            for (j = 0; j < matTaper.rows(); j++) {
                vecInputFFT = rowData.cwiseProduct(matTaper.row(j));
                fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
                matTapSpectrum.row(j) = vecTmpFreq;
            }

            finalResult.push_back(matTapSpectrum);
        }

    } else {
        // Parallel; each worker fills an interleaved set of rows, so the order is kept
        const std::size_t rows = static_cast<std::size_t>(matData.rows());
        finalResult.resize(rows);
        const std::size_t workers = std::max<std::size_t>(1, std::min<std::size_t>(std::thread::hardware_concurrency(), rows));
        std::vector<std::future<void>> tasks;
        tasks.reserve(workers);
        for (std::size_t w = 0; w < workers; ++w) {
            tasks.push_back(std::async(std::launch::async, [&, w] {
                for (std::size_t i = w; i < rows; i += workers) {
                    TaperedSpectraInputData input;
                    input.vecData = matData.row(static_cast<Eigen::Index>(i));
                    input.matTaper = matTaper;
                    input.iNfft = iNfft;
                    finalResult[i] = compute(input);
                }
            }));
        }
        for (auto& task : tasks) {
            task.get();
        }
    }

    return finalResult;
}

//=============================================================================================================

MatrixXcd Spectral::compute(const TaperedSpectraInputData& inputData)
{
    return computeTaperedSpectraRow(inputData.vecData,
                                    inputData.matTaper,
                                    inputData.iNfft);
}

//=============================================================================================================

Eigen::RowVectorXd Spectral::psdFromTaperedSpectra(const Eigen::MatrixXcd& matTapSpectrum,
                                                   const Eigen::VectorXd& vecTapWeights,
                                                   int iNfft,
                                                   double dSampFreq)
{
    //Check inputs
    if (matTapSpectrum.rows() != vecTapWeights.rows()) {
        return Eigen::RowVectorXd();
    }

    //Compute PSD (average over tapers if necessary)
    //Normalization via sFreq
    //multiply by 2 due to half spectrum
    double denom = vecTapWeights.cwiseAbs2().sum() * dSampFreq;
    Eigen::RowVectorXd vecPsd = 2.0 * (vecTapWeights.asDiagonal() * matTapSpectrum).cwiseAbs2().colwise().sum() / denom;

    vecPsd(0) /= 2.0;
    if (iNfft % 2 == 0) {
        vecPsd.tail(1) /= 2.0;
    }

    return vecPsd;
}

//=============================================================================================================

Eigen::RowVectorXcd Spectral::csdFromTaperedSpectra(const Eigen::MatrixXcd& vecTapSpectrumSeed,
                                                    const Eigen::MatrixXcd& vecTapSpectrumTarget,
                                                    const Eigen::VectorXd& vecTapWeightsSeed,
                                                    const Eigen::VectorXd& vecTapWeightsTarget,
                                                    int iNfft,
                                                    double dSampFreq)
{
    //Check inputs
    if (vecTapSpectrumSeed.rows() != vecTapSpectrumTarget.rows()) {
        return Eigen::MatrixXcd();
    }
    if (vecTapSpectrumSeed.cols() != vecTapSpectrumTarget.cols()) {
        return Eigen::MatrixXcd();
    }
    if (vecTapSpectrumSeed.rows() != vecTapWeightsSeed.rows()) {
        return Eigen::MatrixXcd();
    }
    if (vecTapSpectrumTarget.rows() != vecTapWeightsTarget.rows()) {
        return Eigen::MatrixXcd();
    }


    // Compute PSD (average over tapers if necessary)
    // Multiply by 2 due to half spectrum
    // Normalize via sFreq
    double denom = sqrt(vecTapWeightsSeed.cwiseAbs2().sum()) * sqrt(vecTapWeightsTarget.cwiseAbs2().sum()) * dSampFreq;
    Eigen::RowVectorXcd vecCsd = 2.0 * (vecTapWeightsSeed.asDiagonal() * vecTapSpectrumSeed).cwiseProduct((vecTapWeightsTarget.asDiagonal() * vecTapSpectrumTarget).conjugate()).colwise().sum() / denom;


    //multiply first and last element by 2 due to half spectrum
    vecCsd(0) /= 2.0;
    if (iNfft % 2 == 0) {
        vecCsd.tail(1) /= 2.0;
    }


    return vecCsd;
}

//=============================================================================================================

VectorXd Spectral::calculateFFTFreqs(int iNfft, double dSampFreq)
{
    //Compute FFT frequencies
    RowVectorXd vecFFTFreqs;
    if (iNfft % 2 == 0) {
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced(iNfft / 2 + 1, 0.0, iNfft / 2.0);
    } else {
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced((iNfft - 1) / 2 + 1, 0.0, (iNfft - 1) / 2.0);
    }
    return vecFFTFreqs;
}

//=============================================================================================================

std::pair<MatrixXd, VectorXd> Spectral::generateTapers(int iSignalLength, const std::string& sWindowType)
{
    std::pair<MatrixXd, VectorXd> pairOut;
    if (sWindowType == "hanning") {
        pairOut.first = hanningWindow(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    } else if (sWindowType == "ones") {
        pairOut.first = MatrixXd::Ones(1, iSignalLength) / double(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    } else {
        pairOut.first = hanningWindow(iSignalLength);
        pairOut.second = VectorXd::Ones(1);
    }
    return pairOut;
}

//=============================================================================================================

MatrixXd Spectral::hanningWindow(int iSignalLength)
{
    MatrixXd matHann = MatrixXd::Zero(1, iSignalLength);

    //Main step of building the hanning window
    for (int n = 0; n < iSignalLength; n++) {
        matHann(0, n) = 0.5 - 0.5 * cos(2.0 * M_PI * n / (iSignalLength - 1.0));
    }
    matHann.array() /= matHann.row(0).norm();

    return matHann;
}
