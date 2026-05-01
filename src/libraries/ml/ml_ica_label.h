//=============================================================================================================
/**
 * @file     ml_ica_label.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.3.0
 * @date     May, 2026
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
 * @brief    MlIcaLabel class for automatic ICA component classification.
 */

#ifndef ML_ICA_LABEL_H
#define ML_ICA_LABEL_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_global.h"

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
// DEFINE NAMESPACE MLLIB
//=============================================================================================================

namespace MLLIB
{

//=============================================================================================================
/**
 * @brief Label assigned to an ICA component.
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
 * @brief Result of labeling one ICA component.
 */
struct MLSHARED_EXPORT IcaLabelResult
{
    int componentIndex;         /**< 0-based component index. */
    IcaComponentLabel label;    /**< Assigned label. */
    double confidence;          /**< Confidence score in [0, 1]. */

    static QString labelToString(IcaComponentLabel label);
};

//=============================================================================================================
/**
 * @brief Automatic ICA component labeling via correlation with reference signals.
 *
 * Classifies ICA components by correlating each source time course with EOG and ECG
 * reference channels.  Components with high temporal correlation to a reference are
 * labeled accordingly; remaining components are classified by their spectral profile.
 *
 * @code
 *   QList<IcaLabelResult> labels = MlIcaLabel::classify(
 *       icaSources, eogData, ecgData, sFreq);
 *   QVector<int> artIdx = MlIcaLabel::findArtifactComponents(labels);
 * @endcode
 */
class MLSHARED_EXPORT MlIcaLabel
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

} // namespace MLLIB

#endif // ML_ICA_LABEL_H
