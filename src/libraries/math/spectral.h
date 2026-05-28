//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file spectral.h
 * @since March 2026
 * @brief Multi-taper spectral estimation: tapered FFT, power and cross-spectral density, DPSS weighting.
 *
 * @ref UTILSLIB::Spectral implements the multi-taper / Welch family of
 * spectral estimators that the connectivity and time-frequency pipelines
 * in mne-cpp rely on. A single segment of length @c N is windowed by
 * each of @c K Slepian (DPSS) tapers, FFT'd to length @c iNfft, and the
 * @c K complex spectra are combined into a power spectral density (PSD)
 * or a cross-spectral density (CSD) matrix; multi-tapering reduces the
 * variance of the estimator by roughly @c 1/K at the cost of
 * @c (K+1)/(NW) of spectral resolution.
 *
 * The class is static and operates row-wise on @c Eigen matrices so it
 * can fan out across channels via @c QtConcurrent without touching any
 * shared state. PSD scaling follows the one-sided convention
 * (factor 2 on bins 1…Nyquist-1, no doubling at DC or Nyquist) so
 * results are directly comparable to MNE-Python's @c psd_array_multitaper,
 * and CSD matrices are returned as one complex matrix per frequency bin
 * for direct ingestion by CONNECTIVITYLIB's coherence and phase
 * estimators.
 *
 * Reference: Percival & Walden (1993) "Spectral Analysis for Physical
 * Applications", chapters 7–8.
 */

#ifndef SPECTRAL_H
#define SPECTRAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

#include <vector>
#include <utility>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>
#include <QPair>
#include <QSharedPointer>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

/**
 * @brief Per-row input bundle for parallel multi-taper spectral estimation (data row, taper matrix, FFT length).
 */
struct TaperedSpectraInputData {
    Eigen::RowVectorXd vecData;
    Eigen::MatrixXd matTaper;
    int iNfft;
};

//=============================================================================================================
/**
 * Static multi-taper spectral estimator producing tapered spectra, PSDs
 * and cross-spectral density matrices over @c Eigen row inputs. Designed
 * for fan-out across channels via @c QtConcurrent and feeding
 * CONNECTIVITYLIB's coherence / phase estimators.
 *
 * @brief Static multi-taper spectra, PSD and CSD estimator for MEG/EEG time series.
 */
class MATHSHARED_EXPORT Spectral
{

public:
    //=========================================================================================================
    /**
     * deleted default constructor (static class).
     */
    Spectral() = delete;

    //=========================================================================================================
    /**
     * Calculates the full tapered spectra of a given input row data
     *
     * @param[in] vecData         input roww data (time domain), for which the spectrum is computed.
     * @param[in] matTaper        tapers used to compute the spectra.
     * @param[in] iNfft           FFT length.
     *
     * @return tapered spectra of the input data.
     */
    static Eigen::MatrixXcd computeTaperedSpectraRow(const Eigen::RowVectorXd &vecData,
                                                     const Eigen::MatrixXd &matTaper,
                                                     int iNfft);

    //=========================================================================================================
    /**
     * Calculates the full tapered spectra of a given input matrix data. This function calculates each row in parallel.
     *
     * @param[in] matData         input matrix data (time domain), for which the spectrum is computed.
     * @param[in] matTaper        tapers used to compute the spectra.
     * @param[in] iNfft           FFT length.
     * @param[in] bUseThreads Whether to use multiple threads.
     *
     * @return tapered spectra of the input data.
     */
    static QVector<Eigen::MatrixXcd> computeTaperedSpectraMatrix(const Eigen::MatrixXd &matData,
                                                                 const Eigen::MatrixXd &matTaper,
                                                                 int iNfft,
                                                                 bool bUseThreads = true);

    //=========================================================================================================
    /**
     * Computes the tapered spectra for a row vector. This function gets called in parallel.
     *
     * @param[in] inputData    The input data.
     *
     * @return                 The tapered spectra for one data row.
     */
    static Eigen::MatrixXcd compute(const TaperedSpectraInputData& inputData);

    //=========================================================================================================
    /**
     * Reduces the taperedSpectra results to a final result. This function gets called in parallel.
     *
     * @param[out] finalData    The final data data.
     * @param[in] resultData   The resulting data from the computation step.
     */
    static void reduce(QVector<Eigen::MatrixXcd>& finalData,
                       const Eigen::MatrixXcd& resultData);

    //=========================================================================================================
    /**
     * Calculates the power spectral density of given tapered spectrum
     *
     * @param[in] vecTapSpectrum    tapered spectrum, for which the PSD is calculated.
     * @param[in] vecTapWeights     taper weights.
     * @param[in] iNfft             FFT length.
     * @param[in] dSampFreq         sampling frequency of the input data.
     *
     * @return power spectral density of a given tapered spectrum.
     */
    static Eigen::RowVectorXd psdFromTaperedSpectra(const Eigen::MatrixXcd &matTapSpectrum,
                                                    const Eigen::VectorXd &vecTapWeights,
                                                    int iNfft,
                                                    double dSampFreq=1.0);

    //=========================================================================================================
    /**
     * Calculates the cross-spectral density of the tapered spectra of seed and target
     *
     * @param[in] vecTapSpectrumSeed      tapered spectrum of the seed.
     * @param[in] vecTapSpectrumTarget    tapered spectrum of the target.
     * @param[in] vecTapWeightsSeed       taper weights of the seed.
     * @param[in] vecTapWeightsTarget     taper weights of the target.
     * @param[in] iNfft                   FFT length.
     * @param[in] dSampFreq               sampling frequency of the input data.
     *
     * @return cross-spectral density of the tapered spectra of seed and target.
     */
    static Eigen::RowVectorXcd csdFromTaperedSpectra(const Eigen::MatrixXcd &vecTapSpectrumSeed,
                                                     const Eigen::MatrixXcd &vecTapSpectrumTarget,
                                                     const Eigen::VectorXd &vecTapWeightsSeed,
                                                     const Eigen::VectorXd &vecTapWeightsTarget,
                                                     int iNfft,
                                                     double dSampFreq = 1.0);

    //=========================================================================================================
    /**
     * Calculates the FFT frequencies
     *
     * @param[in] iNfft            FFT length.
     * @param[in] dSampFreq        sampling frequency of the input data.
     *
     * @return FFT frequencies.
     */
    static Eigen::VectorXd calculateFFTFreqs(int iNfft, double dSampFreq);

    //=========================================================================================================
    /**
     * Calculates a hanning window of given length
     *
     * @param[in] iSignalLength    length of the hanning window.
     * @param[in] sWindowType      type of the window function used to compute tapered spectra.
     *
     * @return Qpair of tapers and taper weights.
     */
    static QPair<Eigen::MatrixXd, Eigen::VectorXd> generateTapers(int iSignalLength,
                                                                  const QString &sWindowType = "hanning");

    //=========================================================================================================
    /**
     * Calculates a hanning window of given length
     *
     * @param[in] iSignalLength    length of the hanning window.
     * @param[in] sWindowType      type of the window function used to compute tapered spectra.
     *
     * @return Qpair of tapers and taper weights.
     */
    static std::pair<Eigen::MatrixXd, Eigen::VectorXd> generateTapers(int iSignalLength,
                                                                  const std::string &sWindowType = "hanning");

private:
    //=========================================================================================================
    /**
     * Calculates a hanning window of given length
     *
     * @param[in] iSignalLength     length of the hanning window.
     *
     * @return hanning window.
     */
    static Eigen::MatrixXd hanningWindow(int iSignalLength);
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
}//namespace

#endif // SPECTRAL_H
