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
 * above a threshold as BAD_muscle. @ref UTILSLIB::annotateAmplitude "annotateAmplitude" marks
 * spans where consecutive samples jump by more than a "peak" threshold or
 * change by less than a "flat" one, and returns channels for which that
 * holds too often as bad.
 *
 * Both routines reproduce their MNE-Python counterparts
 * @c mne.preprocessing.annotate_muscle_zscore and
 * @c mne.preprocessing.annotate_amplitude onset for onset.
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
#include <QStringList>

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <limits>
#include <optional>

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
    std::optional<double> peak;  /**< Annotate where consecutive samples differ by at least this much (mne's peak); unset = off. */
    std::optional<double> flat;  /**< Annotate where consecutive samples differ by at most this much (mne's flat); unset = off. */
    double dBadPercent = 5.0;    /**< A channel above or below a threshold for at least this percentage of the recording is returned as bad. */
    double dMinDuration = 0.005; /**< Runs of supra- or sub-threshold differences shorter than this (seconds) are ignored. */
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
 * @brief Annotate spans where consecutive samples jump or stay flat, as mne.preprocessing.annotate_amplitude.
 *
 * For every data channel that is not bad, the absolute differences between consecutive samples
 * are compared with params.peak (at least) and params.flat (at most); runs shorter than
 * params.dMinDuration are ignored. A channel whose runs cover at least params.dBadPercent of the
 * recording is returned in @p bads; the runs of the other channels are merged into "BAD_flat"
 * and "BAD_peak" annotations. Unlike mne, a channel that is bad for both reasons is listed once,
 * and BAD_ACQ_SKIP spans are not skipped (the data matrix carries no annotations).
 *
 * @param[in]  data    Raw data matrix (n_channels × n_times).
 * @param[in]  info    Measurement info (channel types, names and bads).
 * @param[in]  sfreq   Sampling frequency in Hz.
 * @param[in]  params  Thresholds; at least one of peak and flat must be set.
 * @param[out] bads    If not null, receives the channels annotated for too long, in channel order.
 * @return "BAD_flat" and "BAD_peak" annotations.
 */
DSPSHARED_EXPORT FIFFLIB::FiffAnnotations annotateAmplitude(
    const Eigen::MatrixXd& data,
    const FIFFLIB::FiffInfo& info,
    double sfreq,
    const AnnotateAmplitudeParams& params,
    QStringList* bads = nullptr);

} // namespace UTILSLIB
#endif // ANNOTATE_ARTIFACT_DSP_H
