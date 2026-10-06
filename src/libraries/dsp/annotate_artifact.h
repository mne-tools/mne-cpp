//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     annotate_artifact.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Continuous-data annotation of muscle and amplitude artefacts.
 *
 * The detectors in this header scan continuous MEG / EEG data and emit
 * @ref FIFFLIB::FiffAnnotation intervals marking time segments that
 * should be excluded from downstream averaging, ICA fitting or PSD
 * estimation. @c annotateMusclZscore band-pass filters the signal in
 * the 110–140 Hz range (typical EMG band), z-scores the Hilbert envelope
 * of every channel, combines and smooths the scores and marks samples
 * above a threshold as BAD_muscle. @ref UTILSLIB::annotateAmplitude "annotateAmplitude" flags two boundary
 * conditions instead: per-channel peak-to-peak amplitude exceeding an
 * upper limit ("high-amplitude" artefact) and amplitude falling below a
 * lower limit for longer than a minimum duration ("flat" / dead channel).
 *
 * Both routines mirror the semantics of their MNE-Python counterparts
 * @c mne.preprocessing.annotate_muscle_zscore and
 * @c mne.preprocessing.annotate_amplitude; annotateMusclZscore reproduces
 * mne's onsets and durations sample for sample.
 *
 * @snippet ex_dsp_artifacts/main.cpp annotate_amplitude_usage
 *
 * @snippet ex_dsp_artifacts/main.cpp annotate_muscle_zscore_usage
 */

#ifndef ANNOTATE_ARTIFACT_DSP_H
#define ANNOTATE_ARTIFACT_DSP_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"
#include <fiff/fiff_annotations.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QString>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <limits>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace FIFFLIB
{
class FiffInfo;
}

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Parameters for muscle artifact annotation.
 *
 * @snippet ex_dsp_artifacts/main.cpp annotate_muscle_zscore_usage
 */
struct DSPSHARED_EXPORT AnnotateMusclParams
{
    double dThreshold = 4.0;     /**< Z-score threshold for marking as muscle artifact. */
    double dFilterLow = 110.0;   /**< Lower edge of the muscle band (Hz). */
    double dFilterHigh = 140.0;  /**< Upper edge of the muscle band (Hz). */
    double dMinLengthGood = 0.1; /**< Good stretches shorter than this (seconds) between annotations become bad. */
};

//=============================================================================================================
/**
 * @brief Parameters for amplitude-based annotation.
 *
 * @snippet ex_dsp_artifacts/main.cpp annotate_amplitude_usage
 */
struct DSPSHARED_EXPORT AnnotateAmplitudeParams
{
    double dPeakMin = -std::numeric_limits<double>::infinity(); /**< Min amplitude — annotate if any sample goes below this. */
    double dPeakMax = std::numeric_limits<double>::infinity();  /**< Max amplitude — annotate if any sample exceeds this. */
    double dFlatMin = 0.0;                                      /**< Flatness threshold — annotate if peak-to-peak in a window < this. */
    double dWindowSec = 0.5;                                    /**< Sliding window duration in seconds for flatness check. */
    double dMinDuration = 0.0;                                  /**< Minimum duration of annotation (seconds). */
    QString badDescription = "BAD_amplitude";                   /**< Description string for annotations. */
};

//=============================================================================================================
/**
 * @brief Detect muscle artifacts via high-frequency z-score and annotate bad segments.
 *
 * Algorithm of @c mne.preprocessing.annotate_muscle_zscore with ch_type None:
 * 1. Pick the magnetometers, else the gradiometers, else the EEG channels.
 * 2. Band-pass them with mne's default FIR filter (@ref FirFilter::filterData).
 * 3. Take the Hilbert envelope (FFT length padded to the next 5-smooth size).
 * 4. Z-score every channel over time, sum over channels and divide by sqrt(n_channels).
 * 5. Low-pass the score at 4 Hz with the same FIR filter.
 * 6. Mark samples above the threshold; good stretches shorter than dMinLengthGood become bad.
 *
 * @param[in] data    Raw data matrix (n_channels × n_times).
 * @param[in] info    Measurement info.
 * @param[in] sfreq   Sampling frequency in Hz.
 * @param[in] params  Detection parameters.
 * @param[out] scores If not null, receives the smoothed score per sample (mne's scores_muscle).
 * @return FiffAnnotations with "BAD_muscle" entries.
 */
DSPSHARED_EXPORT FIFFLIB::FiffAnnotations annotateMusclZscore(
    const Eigen::MatrixXd& data,
    const FIFFLIB::FiffInfo& info,
    double sfreq,
    const AnnotateMusclParams& params = AnnotateMusclParams(),
    Eigen::RowVectorXd* scores = nullptr);

//=============================================================================================================
/**
 * @brief Annotate segments where amplitude exceeds thresholds or is too flat.
 *
 * Scans each channel independently:
 * - If any sample exceeds dPeakMax or falls below dPeakMin, that time point is annotated.
 * - If peak-to-peak amplitude in a sliding window < dFlatMin, the window is annotated as "BAD_flat".
 *
 * @param[in] data    Raw data matrix (n_channels × n_times).
 * @param[in] info    Measurement info (for channel names).
 * @param[in] sfreq   Sampling frequency in Hz.
 * @param[in] params  Detection parameters.
 * @return FiffAnnotations with bad entries.
 */
DSPSHARED_EXPORT FIFFLIB::FiffAnnotations annotateAmplitude(
    const Eigen::MatrixXd& data,
    const FIFFLIB::FiffInfo& info,
    double sfreq,
    const AnnotateAmplitudeParams& params = AnnotateAmplitudeParams());

} // namespace UTILSLIB
#endif // ANNOTATE_ARTIFACT_DSP_H
