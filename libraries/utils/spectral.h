//=============================================================================================================
/**
 * @file     spectral.h
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     March, 2018
 *
 * @section  LICENSE
 *
 * Copyright (C) 2018, Daniel Strohmeier, Lorenz Esch. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
 *       following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 *       the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
 *       to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @note Notes:
 * - Some of this code was adapted from mne-python (https://martinos.org/mne) with permission from Alexandre Gramfort.
 * - This code is prepared for adding spectral estimation with multitapers, which is, however not yet supported.
 * - This code only allows FFT based spectral estimation. Time-frequency transforms are not yet supported.
 *
 * @brief    Declaration of Spectral class.
 */

#ifndef SPECTRAL_H
#define SPECTRAL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

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

struct TaperedSpectraInputData {
    Eigen::RowVectorXd vecData;
    Eigen::MatrixXd matTaper;
    int iNfft;
};

//=============================================================================================================
/**
 * Computes spectral measures of input data such as spectra, power spectral density, cross-spectral density.
 *
 * @brief Computes spectral measures of input data.
 */
class UTILSSHARED_EXPORT Spectral
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

//    //=========================================================================================================
//    /**
//     * Calculates the full tapered spectra of a given input matrix data. This function calculates each row in parallel.
//     *
//     * @param[in] matData         input matrix data (time domain), for which the spectrum is computed.
//     * @param[in] matTaper        tapers used to compute the spectra.
//     * @param[in] iNfft           FFT length.
//     * @param[in] bUseThreads Whether to use multiple threads.
//     *
//     * @return tapered spectra of the input data.
//     */
//    static std::vector<Eigen::MatrixXcd> computeTaperedSpectraMatrix(const Eigen::MatrixXd &matData,
//                                                                    const Eigen::MatrixXd &matTaper,
//                                                                    int iNfft,
//                                                                    bool bUseThreads = true);

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

//    //=========================================================================================================
//    /**
//     * Reduces the taperedSpectra results to a final result. This function gets called in parallel.
//     *
//     * @param[out] finalData    The final data data.
//     * @param[in] resultData   The resulting data from the computation step.
//     */
//    static void reduce(std::vector<Eigen::MatrixXcd>& finalData,
//                       const Eigen::MatrixXcd& resultData);

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
