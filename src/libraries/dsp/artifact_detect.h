//=============================================================================================================
/**
 * @file     artifact_detect.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March, 2026
 *
 * @section  LICENSE
 *
 * Copyright (C) 2026, Christoph Dinh. All rights reserved.
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
 *
 * @brief    Declaration of ArtifactDetect — ECG and EOG physiological artifact event detection.
 *
 * ECG detection finds cardiac R-peak events via bandpass filtering (5–40 Hz) followed by
 * an adaptive threshold and local maximum search.  If no dedicated ECG channel is present
 * the algorithm falls back to a synthetic ECG derived from the MEG gradiometer channels.
 *
 * EOG detection finds blink/saccade events by low-pass filtering (1–10 Hz) and applying
 * an absolute amplitude threshold.
 */

#ifndef ARTIFACT_DETECT_DSP_H
#define ARTIFACT_DETECT_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

#include <fiff/fiff_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVector>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/** @brief ECG R-peak detection parameters (defined outside class to avoid Clang/GCC default-argument issues with nested structs). */
struct DSPSHARED_EXPORT ArtifactDetectEcgParams
{
    double dFilterLow    =  5.0;    /**< Bandpass lower cutoff (Hz). */
    double dFilterHigh   = 40.0;    /**< Bandpass upper cutoff (Hz). */
    int    iFilterOrder  =  4;      /**< Butterworth order for the bandpass pre-filter. */
    double dThreshFactor =  0.5;    /**< R-peak threshold = dThreshFactor * (max − min) + min of the filtered signal. */
    double dMinRRSec     =  0.35;   /**< Minimum R–R interval in seconds (caps detection rate at ~170 bpm). */
};

//=============================================================================================================
/** @brief EOG blink / saccade detection parameters (defined outside class to avoid Clang/GCC default-argument issues with nested structs). */
struct DSPSHARED_EXPORT ArtifactDetectEogParams
{
    double dFilterHigh   = 10.0;    /**< Low-pass cutoff (Hz). */
    int    iFilterOrder  =  4;      /**< Butterworth order for the low-pass pre-filter. */
    double dThresholdV   = 150e-6;  /**< Absolute voltage threshold (V). Events are detected when the signal exceeds ±threshold. */
    double dMinGapSec    =  0.3;    /**< Minimum gap between successive events in seconds. */
};

//=============================================================================================================
/**
 * @brief ECG and EOG physiological artifact event detection.
 *
 * @code
 *   // ECG: returns sample indices of R-peaks
 *   QVector<int> rPeaks = ArtifactDetect::detectEcg(matData, fiffInfo, sFreq);
 *
 *   // EOG: returns sample indices of blink/saccade onsets
 *   QVector<int> blinks = ArtifactDetect::detectEog(matData, fiffInfo, sFreq);
 * @endcode
 */
class DSPSHARED_EXPORT ArtifactDetect
{
public:
    using EcgParams = ArtifactDetectEcgParams; /**< Convenience alias so callers can still write ArtifactDetect::EcgParams. */
    using EogParams = ArtifactDetectEogParams; /**< Convenience alias so callers can still write ArtifactDetect::EogParams. */

    //=========================================================================================================
    /**
     * Detect ECG R-peak events.
     *
     * The function searches FiffInfo for a channel with kind == FIFFV_ECG_CH.  If none is found
     * it synthesises an ECG proxy by averaging the absolute gradient-magnetometer data (MEG channels),
     * which often shows a clear cardiac artefact.
     *
     * @param[in] matData   Raw data matrix (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info (channel kinds, names).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] params    Detection parameters.
     *
     * @return              Sample indices (0-based) of detected R-peaks.
     */
    static QVector<int> detectEcg(const Eigen::MatrixXd&  matData,
                                   const FIFFLIB::FiffInfo& fiffInfo,
                                   double                   dSFreq,
                                   const EcgParams&         params = EcgParams());

    //=========================================================================================================
    /**
     * Detect EOG blink/saccade events.
     *
     * The function searches FiffInfo for channels with kind == FIFFV_EOG_CH.  If multiple EOG
     * channels are present the channel with the largest peak-to-peak amplitude is used.
     * Returns an empty vector if no EOG channel is found.
     *
     * @param[in] matData   Raw data matrix (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info.
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] params    Detection parameters.
     *
     * @return              Sample indices (0-based) of detected EOG events (onset of supra-threshold excursion).
     */
    static QVector<int> detectEog(const Eigen::MatrixXd&  matData,
                                   const FIFFLIB::FiffInfo& fiffInfo,
                                   double                   dSFreq,
                                   const EogParams&         params = EogParams());

private:
    //=========================================================================================================
    /**
     * Band-pass or low-pass filter a single channel row using a zero-phase Butterworth filter.
     *
     * @param[in] vecSignal   Input row vector.
     * @param[in] dSFreq      Sampling frequency.
     * @param[in] dLow        Lower cutoff (Hz). Pass 0 to use a low-pass filter.
     * @param[in] dHigh       Upper cutoff (Hz).
     * @param[in] iOrder      Butterworth filter order.
     *
     * @return Filtered row vector.
     */
    static Eigen::RowVectorXd bandpassFilter(const Eigen::RowVectorXd& vecSignal,
                                              double                    dSFreq,
                                              double                    dLow,
                                              double                    dHigh,
                                              int                       iOrder);

    //=========================================================================================================
    /**
     * Local-maximum peak picking above a threshold with a minimum inter-peak distance.
     *
     * @param[in] vecSignal   1D signal.
     * @param[in] dThreshold  Minimum peak height.
     * @param[in] iMinDist    Minimum number of samples between successive peaks.
     *
     * @return Sample indices of detected peaks.
     */
    static QVector<int> findPeaks(const Eigen::RowVectorXd& vecSignal,
                                   double                    dThreshold,
                                   int                       iMinDist);
};

} // namespace UTILSLIB

#endif // ARTIFACT_DETECT_DSP_H
