//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     artifact_detect.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.1.0
 * @date     March 2026
 * @brief    Declaration of ArtifactDetect — ECG and EOG physiological artifact event detection.
 *
 * ECG detection is mne.preprocessing.find_ecg_events: a 5–35 Hz zero-double firwin2 band-pass and the
 * qrs_detector window search; without an ECG channel the mean of the magnetometers (or gradiometers)
 * stands in for it.
 *
 * EOG detection is mne.preprocessing.find_eog_events: the strongest EOG channel, band-passed 1–10 Hz,
 * searched with mne.preprocessing.peak_finder.
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
/**
 * @brief ECG R-peak detection parameters (defined outside class to avoid Clang/GCC default-argument issues with nested structs).
 *
 * @snippet ex_dsp_artifacts/main.cpp artifact_detect_usage
 */
struct DSPSHARED_EXPORT ArtifactDetectEcgParams
{
    double dFilterLow = 5.0;     /**< Band-pass lower edge in Hz (l_freq). */
    double dFilterHigh = 35.0;   /**< Band-pass upper edge in Hz (h_freq). */
    double dQrsThreshold = -1.0; /**< QRS threshold in (0, 1]; negative = "auto" (0.3 ... 1.05, rate closest to the median plausible one). */
    double dTStart = 0.0;        /**< Seconds to skip at the start (tstart). */
};

//=============================================================================================================
/**
 * @brief EOG blink / saccade detection parameters (defined outside class to avoid Clang/GCC default-argument issues with nested structs).
 *
 * @snippet ex_dsp_artifacts/main.cpp artifact_detect_usage
 */
struct DSPSHARED_EXPORT ArtifactDetectEogParams
{
    double dFilterLow = 1.0;                                      /**< Band-pass lower edge in Hz (l_freq). */
    double dFilterHigh = 10.0;                                    /**< Band-pass upper edge in Hz (h_freq). */
    double dThreshold = std::numeric_limits<double>::quiet_NaN(); /**< peak_finder threshold; NaN = (max - min) / 4. */
    double dTStart = 0.0;                                         /**< Seconds to skip at the start (tstart). */
};

//=============================================================================================================
/**
 * @brief ECG and EOG physiological artifact event detection.
 *
 * @snippet ex_dsp_artifacts/main.cpp artifact_detect_usage
 */
class DSPSHARED_EXPORT ArtifactDetect
{
public:
    using EcgParams = ArtifactDetectEcgParams; /**< Convenience alias so callers can still write ArtifactDetect::EcgParams. */
    using EogParams = ArtifactDetectEogParams; /**< Convenience alias so callers can still write ArtifactDetect::EogParams. */

    //=========================================================================================================
    /**
     * Detect ECG R-peak events as mne.preprocessing.find_ecg_events does.
     *
     * Uses the first good channel with kind == FIFFV_ECG_CH; without one, the mean of the good
     * magnetometers (or, failing those, gradiometers) stands in for it.
     *
     * @param[in] matData   Raw data matrix (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info (channel kinds, names).
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] params    Detection parameters.
     *
     * @return              Sample indices (0-based) of detected R-peaks.
     */
    static QVector<int> detectEcg(const Eigen::MatrixXd& matData,
                                  const FIFFLIB::FiffInfo& fiffInfo,
                                  double dSFreq,
                                  const EcgParams& params = EcgParams());

    //=========================================================================================================
    /**
     * Detect EOG blink/saccade events as mne.preprocessing.find_eog_events does.
     *
     * Of the good channels with kind == FIFFV_EOG_CH, the one with the most 2-45 Hz energy is used.
     * Returns an empty vector if no EOG channel is found.
     *
     * @param[in] matData   Raw data matrix (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info.
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] params    Detection parameters.
     *
     * @return              Sample indices (0-based) of the detected EOG peaks.
     */
    static QVector<int> detectEog(const Eigen::MatrixXd& matData,
                                  const FIFFLIB::FiffInfo& fiffInfo,
                                  double dSFreq,
                                  const EogParams& params = EogParams());

private:
    /** R-peak search of mne.preprocessing.qrs_detector on an already filtered ECG. */
    static QVector<int> qrsDetector(const Eigen::RowVectorXd& filtered, double dSFreq, double dThreshold, double dTStart);
};

} // namespace UTILSLIB

#endif // ARTIFACT_DETECT_DSP_H
