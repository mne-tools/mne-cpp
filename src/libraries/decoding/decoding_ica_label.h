//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file decoding_ica_label.h
 * @since May 2026
 * @brief Automatic ICA component labelling for artefact identification on M/EEG.
 *
 * After an independent-component analysis decomposition of an M/EEG
 * recording the analyst is left with one source time course per
 * component and must decide which sources represent brain activity and
 * which are physiological or technical artefacts (eye blinks, lateral
 * eye movements, heart-beat, muscle tonus, line noise). Doing that by
 * hand is the dominant bottleneck of the cleaning pipeline; this
 * module replaces it with a deterministic, reference-driven
 * classifier that is fast enough to run in the GUI on every fit.
 *
 * The strategy is the same as the classic correlation-with-references
 * approach used in @c mne.preprocessing.ICA.find_bads_eog /
 * @c find_bads_ecg: for each component the maximum absolute Pearson
 * correlation with the (possibly multi-channel) EOG and ECG reference
 * traces is computed and compared against per-modality thresholds. A
 * spectral heuristic estimating the fraction of power above 30 Hz is
 * additionally used to flag muscle components, which dominate the
 * high-frequency end of the EEG spectrum. The output is one
 * @ref IcaLabelResult per component carrying the assigned
 * @ref IcaComponentLabel and a confidence score in @f$[0, 1]@f$ that
 * the higher-level workflow can threshold or visualise. The class is
 * a static-only utility — there is no fitted state and no learned
 * model file — which makes the labelling fully reproducible.
 */

#ifndef DECODING_ICA_LABEL_H
#define DECODING_ICA_LABEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "decoding_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QString>
#include <QPair>

//=============================================================================================================
// DEFINE NAMESPACE DECODINGLIB
//=============================================================================================================

namespace DECODINGLIB
{

//=============================================================================================================
/**
 * @brief Categorical label assigned to a single ICA component.
 *
 * Spans the four artefact families that account for essentially all
 * non-brain variance in scalp M/EEG — ocular, cardiac and muscular —
 * plus a generic @c Other bucket for components that fail every
 * specific check (line noise, channel pops, residual sensor jumps).
 * @c Brain is the default and means "keep this component in the
 * reconstruction".
 */
enum class IcaComponentLabel
{
    Brain,
    Eog,
    Ecg,
    Muscle,
    Other
};

//=============================================================================================================
/**
 * @brief Outcome of labelling a single ICA component.
 *
 * Pairs a component index with its winning @ref IcaComponentLabel and a
 * confidence score in @f$[0, 1]@f$ derived from the underlying
 * correlation or spectral statistic (for EOG/ECG: the maximum absolute
 * Pearson correlation with the reference traces; for muscle: the
 * fraction of power above 30 Hz). Downstream consumers typically
 * threshold the score or render it next to the component topography so
 * the user can override the automatic decision.
 */
struct DECODINGSHARED_EXPORT IcaLabelResult
{
    int componentIndex;         /**< 0-based component index. */
    IcaComponentLabel label;    /**< Assigned label. */
    double confidence;          /**< Confidence score in [0, 1]. */

    static QString labelToString(IcaComponentLabel label);
};

//=============================================================================================================
/**
 * @brief Static utility that labels ICA components against EOG/ECG references and a muscle spectral heuristic.
 *
 * @ref classify walks the rows of the source matrix and applies, in
 * order, the EOG correlation test, the ECG correlation test and the
 * high-frequency muscle test; the first one whose score exceeds its
 * threshold wins, otherwise the component is labelled @c Brain. The
 * thresholds default to the canonical 0.3 used by the MNE-Python
 * helpers and can be tightened or loosened per call. @ref findArtifactComponents
 * is the convenience that returns just the indices of the rejected
 * components in the order expected by the ICA reconstruction code,
 * making it a one-liner to wire automatic cleaning into a real-time
 * pipeline.
 *
 * The two scoring primitives — @ref maxAbsCorrelation and
 * @ref muscleScore — are exposed publicly so an interactive viewer can
 * display the per-component evidence behind each decision and let the
 * user override the label without having to recompute the ICA itself.
 * The class has no state, deleted constructor, and no virtual methods;
 * it is purely a namespaced collection of pure functions.
 *
 * @code
 *   QList<IcaLabelResult> labels = MlIcaLabel::classify(
 *       icaSources, eogData, ecgData, sFreq);
 *   QVector<int> artIdx = MlIcaLabel::findArtifactComponents(labels);
 * @endcode
 */
class DECODINGSHARED_EXPORT MlIcaLabel
{
public:
    //=========================================================================================================
    /**
     * Classify each ICA component using reference signals and spectral heuristics.
     *
     * @param[in] matSources   ICA sources (n_components x n_samples).
     * @param[in] matEog       EOG reference channels (n_eog x n_samples). Can be empty.
     * @param[in] matEcg       ECG reference channels (n_ecg x n_samples). Can be empty.
     * @param[in] dSFreq       Sampling frequency in Hz.
     * @param[in] dEogThresh   Correlation threshold for EOG classification (default 0.3).
     * @param[in] dEcgThresh   Correlation threshold for ECG classification (default 0.3).
     *
     * @return Label results for each component.
     */
    static QList<IcaLabelResult> classify(const Eigen::MatrixXd& matSources,
                                           const Eigen::MatrixXd& matEog,
                                           const Eigen::MatrixXd& matEcg,
                                           double dSFreq,
                                           double dEogThresh = 0.3,
                                           double dEcgThresh = 0.3);

    //=========================================================================================================
    /**
     * Extract indices of artifact components (EOG, ECG, muscle).
     *
     * @param[in] labels   Label results from classify().
     *
     * @return 0-based indices of artifact components.
     */
    static QVector<int> findArtifactComponents(const QList<IcaLabelResult>& labels);

    //=========================================================================================================
    /**
     * Compute the maximum absolute Pearson correlation between a source and reference channels.
     *
     * @param[in] source   Source time course (n_samples).
     * @param[in] matRef   Reference channels (n_ref x n_samples).
     *
     * @return Maximum |r| across reference channels.
     */
    static double maxAbsCorrelation(const Eigen::VectorXd& source,
                                     const Eigen::MatrixXd& matRef);

    //=========================================================================================================
    /**
     * Compute a simple spectral power ratio heuristic for muscle artifact detection.
     * Muscle artifacts typically have high power above 30 Hz.
     *
     * @param[in] source   Source time course (n_samples).
     * @param[in] dSFreq   Sampling frequency in Hz.
     *
     * @return Ratio of high-frequency to total power (0 = no muscle, 1 = all muscle).
     */
    static double muscleScore(const Eigen::VectorXd& source, double dSFreq);

private:
    MlIcaLabel() = delete;
};

} // namespace DECODINGLIB

#endif // DECODING_ICA_LABEL_H
