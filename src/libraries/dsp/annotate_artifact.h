//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file annotate_artifact.h
 * @since 2026
 * @date  May 2026
 * @brief Continuous-data annotation of muscle and amplitude artefacts.
 *
 * The detectors in this header scan continuous MEG / EEG data and emit
 * @ref FIFFLIB::FiffAnnotation intervals marking time segments that
 * should be excluded from downstream averaging, ICA fitting or PSD
 * estimation. @ref annotateMuscleZscore band-pass filters the signal in
 * the 110–140 Hz range (typical EMG band), takes the smoothed envelope
 * across channels and z-scores it against the global median absolute
 * deviation; samples whose z-score exceeds a user threshold are merged
 * into BAD_muscle annotations. @ref annotateAmplitude flags two boundary
 * conditions instead: per-channel peak-to-peak amplitude exceeding an
 * upper limit ("high-amplitude" artefact) and amplitude falling below a
 * lower limit for longer than a minimum duration ("flat" / dead channel).
 *
 * Both routines mirror the semantics of their MNE-Python counterparts
 * @c mne.preprocessing.annotate_muscle_zscore and
 * @c mne.preprocessing.annotate_amplitude so cross-toolchain pipelines
 * produce identical bad-segment lists.
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

namespace FIFFLIB { class FiffInfo; }

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB {

//=============================================================================================================
/**
 * @brief Parameters for muscle artifact annotation.
 */
struct DSPSHARED_EXPORT AnnotateMusclParams
{
    double dThreshold    = 5.0;     /**< Z-score threshold for marking as muscle artifact. */
    double dFilterLow    = 110.0;   /**< High-pass cutoff for muscle band (Hz). */
    double dFilterHigh   = 140.0;   /**< Low-pass cutoff for muscle band (Hz). */
    int    iFilterOrder  = 4;       /**< Butterworth order. */
    double dMinDuration  = 0.1;     /**< Minimum annotation duration in seconds. */
    double dMinGapSec    = 0.25;    /**< Merge annotations closer than this (seconds). */
};

//=============================================================================================================
/**
 * @brief Parameters for amplitude-based annotation.
 */
struct DSPSHARED_EXPORT AnnotateAmplitudeParams
{
    double dPeakMin = -std::numeric_limits<double>::infinity(); /**< Min amplitude — annotate if any sample goes below this. */
    double dPeakMax =  std::numeric_limits<double>::infinity(); /**< Max amplitude — annotate if any sample exceeds this. */
    double dFlatMin = 0.0;    /**< Flatness threshold — annotate if peak-to-peak in a window < this. */
    double dWindowSec = 0.5;  /**< Sliding window duration in seconds for flatness check. */
    double dMinDuration = 0.0; /**< Minimum duration of annotation (seconds). */
    QString badDescription = "BAD_amplitude"; /**< Description string for annotations. */
};

//=============================================================================================================
/**
 * @brief Detect muscle artifacts via high-frequency z-score and annotate bad segments.
 *
 * Algorithm:
 * 1. Bandpass filter data in muscle frequency band (default 110–140 Hz).
 * 2. Compute Hilbert envelope (approximated via absolute value of filtered signal).
 * 3. Compute z-score of the envelope across time for each MEG channel.
 * 4. Average z-score across channels.
 * 5. Threshold: mark time points where average z-score > threshold.
 * 6. Merge adjacent annotations and apply minimum duration.
 *
 * @param[in] data    Raw data matrix (n_channels × n_times).
 * @param[in] info    Measurement info.
 * @param[in] sfreq   Sampling frequency in Hz.
 * @param[in] params  Detection parameters.
 * @return FiffAnnotations with "BAD_muscle" entries.
 */
DSPSHARED_EXPORT FIFFLIB::FiffAnnotations annotateMusclZscore(
    const Eigen::MatrixXd& data,
    const FIFFLIB::FiffInfo& info,
    double sfreq,
    const AnnotateMusclParams& params = AnnotateMusclParams());

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
