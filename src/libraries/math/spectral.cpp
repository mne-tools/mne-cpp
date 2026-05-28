//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file spectral.cpp
 * @since March 2026
 * @brief FFT, multi-taper combination and PSD/CSD assembly kernels.
 *
 * Implements the static estimators declared in @ref spectral.h. Each
 * tapered FFT runs through @c Eigen::FFT (Kiss-FFT backend) at
 * @c O(N log N); the @c K taper streams are then averaged into a PSD
 * with the one-sided scaling convention or combined into a complex
 * cross-spectral-density matrix per frequency bin. The matrix entry
 * point uses @c QtConcurrent::mapped to spread channels across worker
 * threads while keeping a deterministic ordering in the output vector.
 *
 * Numerical conventions match MNE-Python's @c psd_array_multitaper and
 * @c csd_array_multitaper so coherence, phase-locking and DICS results
 * computed downstream by CONNECTIVITYLIB remain comparable across the
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
// QT INCLUDES
//=============================================================================================================

#include <QtMath>
#include <QtConcurrent>
#include <QVector>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXcd Spectral::computeTaperedSpectraRow(const RowVectorXd &vecData,
                                             const MatrixXd &matTaper,
                                             int iNfft)
{
    //qDebug() << "Spectral::computeTaperedSpectra Matrixwise";
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
    for (int i=0; i < matTaper.rows(); i++) {
        vecInputFFT = vecData.cwiseProduct(matTaper.row(i));
        fft.fwd(vecTmpFreq, vecInputFFT, iNfft);
        matTapSpectrum.row(i) = vecTmpFreq;
    }

    return matTapSpectrum;
}

//=============================================================================================================

QVector<MatrixXcd> Spectral::computeTaperedSpectraMatrix(const MatrixXd &matData,
                                                         const MatrixXd &matTaper,
                                                         int iNfft,
                                                         bool bUseThreads)
{
    #ifdef EIGEN_FFTW_DEFAULT
        fftw_make_planner_thread_safe();
    #endif

    QVector<MatrixXcd> finalResult;

    if(!bUseThreads) {
        // Sequential
//        QElapsedTimer timer;
//        int iTime = 0;
//        int iTimeAll = 0;
//        timer.start();

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

            finalResult.append(matTapSpectrum);

//            iTime = timer.elapsed();
//            qDebug() << QThread::currentThreadId() << "Spectral::computeTaperedSpectraMatrix - Row-wise computation:" << iTime;
//            iTimeAll += iTime;
//            timer.restart();
        }

//        qDebug() << QThread::currentThreadId() << "Spectral::computeTaperedSpectraMatrix - Complete computation:" << iTimeAll;
    } else {
        // Parallel
        QList<TaperedSpectraInputData> lData;
        TaperedSpectraInputData dataTemp;
        dataTemp.matTaper = matTaper;
        dataTemp.iNfft = iNfft;

        for (int i = 0; i < matData.rows(); ++i) {
            dataTemp.vecData = matData.row(i);

            lData.append(dataTemp);
        }

        QFuture<QVector<MatrixXcd> > result = QtConcurrent::mappedReduced(lData,
                                                                          compute,
                                                                          reduce,
                                                                          QtConcurrent::OrderedReduce);
        result.waitForFinished();
        finalResult = result.result();
    }

    return finalResult;
}

//=============================================================================================================

MatrixXcd Spectral::compute(const TaperedSpectraInputData& inputData)
{
    //qDebug() << "Spectral::compute";
    return computeTaperedSpectraRow(inputData.vecData,
                                    inputData.matTaper,
                                    inputData.iNfft);
}

//=============================================================================================================

void Spectral::reduce(QVector<MatrixXcd>& finalData,
                      const MatrixXcd& resultData)
{
    //qDebug() << "Spectral::reduce";
    finalData.append(resultData);
}

//=============================================================================================================

Eigen::RowVectorXd Spectral::psdFromTaperedSpectra(const Eigen::MatrixXcd &matTapSpectrum,
                                                   const Eigen::VectorXd &vecTapWeights,
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
    if (iNfft % 2 == 0){
        vecPsd.tail(1) /= 2.0;
    }

    return vecPsd;
}

//=============================================================================================================

Eigen::RowVectorXcd Spectral::csdFromTaperedSpectra(const Eigen::MatrixXcd &vecTapSpectrumSeed,
                                                    const Eigen::MatrixXcd &vecTapSpectrumTarget,
                                                    const Eigen::VectorXd &vecTapWeightsSeed,
                                                    const Eigen::VectorXd &vecTapWeightsTarget,
                                                    int iNfft,
                                                    double dSampFreq)
{
//    QElapsedTimer timer;
//    int iTime = 0;
//    timer.start();

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

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Spectral::csdFromTaperedSpectra timer - Prepare:" << iTime;
//    timer.restart();

    // Compute PSD (average over tapers if necessary)
    // Multiply by 2 due to half spectrum
    // Normalize via sFreq
    double denom = sqrt(vecTapWeightsSeed.cwiseAbs2().sum()) * sqrt(vecTapWeightsTarget.cwiseAbs2().sum()) * dSampFreq;
    Eigen::RowVectorXcd vecCsd = 2.0 * (vecTapWeightsSeed.asDiagonal() * vecTapSpectrumSeed).cwiseProduct((vecTapWeightsTarget.asDiagonal() * vecTapSpectrumTarget).conjugate()).colwise().sum() / denom;

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Spectral::csdFromTaperedSpectra timer - compute PSD:" << iTime;
//    timer.restart();

    //multiply first and last element by 2 due to half spectrum
    vecCsd(0) /= 2.0;
    if (iNfft % 2 == 0){
        vecCsd.tail(1) /= 2.0;
    }

//    iTime = timer.elapsed();
//    qDebug() << QThread::currentThreadId() << "Spectral::csdFromTaperedSpectra timer - half spectrum:" << iTime;
//    timer.restart();

    return vecCsd;
}

//=============================================================================================================

VectorXd Spectral::calculateFFTFreqs(int iNfft, double dSampFreq)
{
    //Compute FFT frequencies
    RowVectorXd vecFFTFreqs;
    if (iNfft % 2 == 0){
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced(iNfft / 2.0 + 1, 0.0, iNfft / 2.0);
    } else {
        vecFFTFreqs = (dSampFreq / iNfft) * RowVectorXd::LinSpaced((iNfft - 1) / 2.0 + 1, 0.0, (iNfft - 1) / 2.0);
    }
    return vecFFTFreqs;
}

//=============================================================================================================

QPair<MatrixXd, VectorXd> Spectral::generateTapers(int iSignalLength, const QString &sWindowType)
{
    QPair<MatrixXd, VectorXd> pairOut;
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

std::pair<MatrixXd, VectorXd> Spectral::generateTapers(int iSignalLength, const std::string &sWindowType)
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
