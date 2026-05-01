//=============================================================================================================
/**
 * @file     ml_ica_label.cpp
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
 * @brief    MlIcaLabel class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "ml_ica_label.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MLLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

QString IcaLabelResult::labelToString(IcaComponentLabel label)
{
    switch (label) {
        case IcaComponentLabel::Brain:  return QStringLiteral("brain");
        case IcaComponentLabel::Eog:    return QStringLiteral("eog");
        case IcaComponentLabel::Ecg:    return QStringLiteral("ecg");
        case IcaComponentLabel::Muscle: return QStringLiteral("muscle");
        case IcaComponentLabel::Other:  return QStringLiteral("other");
    }
    return QStringLiteral("unknown");
}

//=============================================================================================================

QList<IcaLabelResult> MlIcaLabel::classify(const MatrixXd& matSources,
                                             const MatrixXd& matEog,
                                             const MatrixXd& matEcg,
                                             double dSFreq,
                                             double dEogThresh,
                                             double dEcgThresh)
{
    QList<IcaLabelResult> results;

    const int nComp = static_cast<int>(matSources.rows());
    if (nComp == 0)
        return results;

    const double dMuscleThresh = 0.7;

    for (int k = 0; k < nComp; ++k) {
        IcaLabelResult res;
        res.componentIndex = k;
        res.label = IcaComponentLabel::Brain;
        res.confidence = 0.0;

        VectorXd source = matSources.row(k).transpose();

        // Check EOG correlation
        double eogCorr = 0.0;
        if (matEog.rows() > 0 && matEog.cols() == matSources.cols())
            eogCorr = maxAbsCorrelation(source, matEog);

        // Check ECG correlation
        double ecgCorr = 0.0;
        if (matEcg.rows() > 0 && matEcg.cols() == matSources.cols())
            ecgCorr = maxAbsCorrelation(source, matEcg);

        // Check muscle score
        double muscle = muscleScore(source, dSFreq);

        // Classification logic: highest evidence wins
        if (eogCorr >= dEogThresh && eogCorr >= ecgCorr && eogCorr >= muscle) {
            res.label = IcaComponentLabel::Eog;
            res.confidence = eogCorr;
        } else if (ecgCorr >= dEcgThresh && ecgCorr >= eogCorr && ecgCorr >= muscle) {
            res.label = IcaComponentLabel::Ecg;
            res.confidence = ecgCorr;
        } else if (muscle >= dMuscleThresh) {
            res.label = IcaComponentLabel::Muscle;
            res.confidence = muscle;
        } else {
            // Default to brain if no artifact criteria met
            res.label = IcaComponentLabel::Brain;
            res.confidence = 1.0 - std::max({eogCorr, ecgCorr, muscle});
        }

        results.append(res);
    }

    return results;
}

//=============================================================================================================

QVector<int> MlIcaLabel::findArtifactComponents(const QList<IcaLabelResult>& labels)
{
    QVector<int> artifacts;
    for (const auto& res : labels) {
        if (res.label == IcaComponentLabel::Eog ||
            res.label == IcaComponentLabel::Ecg ||
            res.label == IcaComponentLabel::Muscle) {
            artifacts.append(res.componentIndex);
        }
    }
    return artifacts;
}

//=============================================================================================================

double MlIcaLabel::maxAbsCorrelation(const VectorXd& source, const MatrixXd& matRef)
{
    const int n = static_cast<int>(source.size());
    if (n < 2)
        return 0.0;

    // Demean source
    double srcMean = source.mean();
    VectorXd srcCentered = source.array() - srcMean;
    double srcStd = std::sqrt(srcCentered.squaredNorm() / static_cast<double>(n - 1));

    if (srcStd < 1e-15)
        return 0.0;

    double maxCorr = 0.0;
    for (int r = 0; r < matRef.rows(); ++r) {
        VectorXd ref = matRef.row(r).transpose();
        double refMean = ref.mean();
        VectorXd refCentered = ref.array() - refMean;
        double refStd = std::sqrt(refCentered.squaredNorm() / static_cast<double>(n - 1));

        if (refStd < 1e-15)
            continue;

        double corr = std::abs(srcCentered.dot(refCentered) / (static_cast<double>(n - 1) * srcStd * refStd));
        if (corr > maxCorr)
            maxCorr = corr;
    }

    return maxCorr;
}

//=============================================================================================================

double MlIcaLabel::muscleScore(const VectorXd& source, double dSFreq)
{
    // Simple time-domain approximation of HF power ratio:
    // Compute variance of first-differenced signal vs original.
    // First-difference acts as a high-pass filter (accentuates high freq).
    const int n = static_cast<int>(source.size());
    if (n < 3 || dSFreq <= 0.0)
        return 0.0;

    double totalVar = 0.0;
    double srcMean = source.mean();
    for (int i = 0; i < n; ++i) {
        double d = source[i] - srcMean;
        totalVar += d * d;
    }
    totalVar /= static_cast<double>(n - 1);

    if (totalVar < 1e-30)
        return 0.0;

    // Compute variance of second difference (approximates d²/dt²)
    double hfVar = 0.0;
    for (int i = 1; i < n - 1; ++i) {
        double d2 = source[i + 1] - 2.0 * source[i] + source[i - 1];
        hfVar += d2 * d2;
    }
    hfVar /= static_cast<double>(n - 2);

    // Normalize: for white noise, hfVar/totalVar → 6.0 (analytical result for second diff of white noise)
    // So ratio = hfVar / (6 * totalVar) gives ~1 for white noise, <1 for smooth signals
    double ratio = hfVar / (6.0 * totalVar);

    return std::min(ratio, 1.0);
}
