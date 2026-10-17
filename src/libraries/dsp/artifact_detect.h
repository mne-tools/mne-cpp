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
#include <fiff/fiff_proj.h>

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
 * @brief SSP parameters of compute_proj_ecg / compute_proj_eog; ecg() and eog() give the MNE-Python defaults.
 *
 * @snippet ex_dsp_artifacts/main.cpp artifact_detect_usage
 */
struct DSPSHARED_EXPORT ArtifactDetectProjParams
{
    double dTmin = -0.2;           /**< Epoch start relative to the event in seconds. */
    double dTmax = 0.4;            /**< Epoch end relative to the event in seconds. */
    int nGrad = 2;                 /**< Number of gradiometer vectors. */
    int nMag = 2;                  /**< Number of magnetometer vectors. */
    int nEeg = 2;                  /**< Number of EEG vectors. */
    double dFilterLow = 1.0;       /**< Band-pass lower edge of the data in Hz (l_freq). */
    double dFilterHigh = 35.0;     /**< Band-pass upper edge of the data in Hz (h_freq). */
    double dGradReject = 2000e-13; /**< Peak-to-peak rejection in T/m; 0 = off. */
    double dMagReject = 3000e-15;  /**< Peak-to-peak rejection in T; 0 = off. */
    double dEegReject = 50e-6;     /**< Peak-to-peak rejection in V; 0 = off. */
    double dEogReject = 250e-6;    /**< Peak-to-peak rejection in V; 0 = off. */

    /** @return The compute_proj_ecg defaults. */
    static ArtifactDetectProjParams ecg()
    {
        return {};
    }

    /** @return The compute_proj_eog defaults (tmax 0.2, eeg 500 µV, no EOG rejection). */
    static ArtifactDetectProjParams eog()
    {
        ArtifactDetectProjParams p;
        p.dTmax = 0.2;
        p.dEegReject = 500e-6;
        p.dEogReject = 0.0;
        return p;
    }
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

    using ProjParams = ArtifactDetectProjParams; /**< Convenience alias. */

    //=========================================================================================================
    /**
     * SSP projectors for heartbeat artifacts as mne.preprocessing.compute_proj_ecg does: ECG events,
     * band-passed data with the info projectors applied, peak-to-peak rejection, average, then
     * FiffProj::compute_from_evoked. Returns the info projectors followed by the new ones ("ECG-...").
     *
     * @param[in] matData   Raw data (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info including its projectors.
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] proj      Epoching, filtering, rejection and vector counts.
     * @param[in] detect    ECG event detection parameters.
     *
     * @return The projectors, or only the info projectors if no event or epoch survives.
     */
    static QList<FIFFLIB::FiffProj> computeProjEcg(const Eigen::MatrixXd& matData,
                                                   const FIFFLIB::FiffInfo& fiffInfo,
                                                   double dSFreq,
                                                   const ProjParams& proj = ProjParams::ecg(),
                                                   const EcgParams& detect = EcgParams());

    //=========================================================================================================
    /**
     * SSP projectors for blink artifacts as mne.preprocessing.compute_proj_eog does (see computeProjEcg).
     *
     * @param[in] matData   Raw data (n_channels × n_samples), calibrated (SI units).
     * @param[in] fiffInfo  Measurement info including its projectors.
     * @param[in] dSFreq    Sampling frequency in Hz.
     * @param[in] proj      Epoching, filtering, rejection and vector counts.
     * @param[in] detect    EOG event detection parameters.
     *
     * @return The projectors, or only the info projectors if no event or epoch survives.
     */
    static QList<FIFFLIB::FiffProj> computeProjEog(const Eigen::MatrixXd& matData,
                                                   const FIFFLIB::FiffInfo& fiffInfo,
                                                   double dSFreq,
                                                   const ProjParams& proj = ProjParams::eog(),
                                                   const EogParams& detect = EogParams());

private:
    /** R-peak search of mne.preprocessing.qrs_detector on an already filtered ECG. */
    static QVector<int> qrsDetector(const Eigen::RowVectorXd& filtered, double dSFreq, double dThreshold, double dTStart);
};

} // namespace UTILSLIB

#endif // ARTIFACT_DETECT_DSP_H
