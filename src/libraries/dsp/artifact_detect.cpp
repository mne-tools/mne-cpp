//=============================================================================================================
/**
 * @file     artifact_detect.cpp
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
 * @brief    Implementation of ArtifactDetect.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "artifact_detect.h"
#include "iirfilter.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

RowVectorXd ArtifactDetect::bandpassFilter(const RowVectorXd& vecSignal,
                                             double             dSFreq,
                                             double             dLow,
                                             double             dHigh,
                                             int                iOrder)
{
    QVector<IirBiquad> sos;

    if (dLow < 1e-3) {
        // Low-pass only
        sos = IirFilter::designButterworth(iOrder, IirFilter::LowPass, dHigh, 0.0, dSFreq);
    } else {
        sos = IirFilter::designButterworth(iOrder, IirFilter::BandPass, dLow, dHigh, dSFreq);
    }

    return IirFilter::applyZeroPhase(vecSignal, sos);
}

//=============================================================================================================

QVector<int> ArtifactDetect::findPeaks(const RowVectorXd& vecSignal,
                                         double             dThreshold,
                                         int                iMinDist)
{
    QVector<int> peaks;
    const int N = static_cast<int>(vecSignal.size());
    if (N < 3) return peaks;

    int lastPeak = -iMinDist - 1;

    for (int i = 1; i < N - 1; ++i) {
        double v = vecSignal(i);
        if (v < dThreshold) continue;

        // Local maximum check
        if (v > vecSignal(i - 1) && v >= vecSignal(i + 1)) {
            // Enforce minimum distance
            if (i - lastPeak >= iMinDist) {
                peaks.append(i);
                lastPeak = i;
            } else if (!peaks.isEmpty() && vecSignal(peaks.last()) < v) {
                // Replace last peak if this one is higher and within the distance window
                peaks.last() = i;
                lastPeak = i;
            }
        }
    }

    return peaks;
}

//=============================================================================================================
// PUBLIC DEFINITIONS
//=============================================================================================================

QVector<int> ArtifactDetect::detectEcg(const MatrixXd&  matData,
                                         const FiffInfo&  fiffInfo,
                                         double           dSFreq,
                                         const EcgParams& params)
{
    // ---- Find ECG channel ----
    int ecgIdx = -1;
    for (int i = 0; i < fiffInfo.nchan; ++i) {
        if (fiffInfo.chs[i].kind == FIFFV_ECG_CH) {
            ecgIdx = i;
            break;
        }
    }

    RowVectorXd ecgSignal;

    if (ecgIdx >= 0) {
        // Use dedicated ECG channel (already calibrated)
        ecgSignal = matData.row(ecgIdx).cast<double>();
    } else {
        // ---- Synthetic ECG from MEG channels ----
        // The cardiac artifact is visible on most MEG channels.  A robust synthetic ECG is
        // obtained by summing the absolute values of all magnetometer channels (gradiometer
        // baseline artefact reduces them; magnetometers show the global field pattern).
        qWarning() << "ArtifactDetect::detectEcg: No ECG channel found. "
                      "Synthesising ECG proxy from MEG magnetometers.";

        QVector<int> magIdx;
        for (int i = 0; i < fiffInfo.nchan; ++i) {
            if (fiffInfo.chs[i].kind == FIFFV_MEG_CH) {
                // Distinguish magnetometers from gradiometers by coil type:
                // magnetometers have a single integration point — heuristically identified
                // as FIFFV_COIL_MAG type; use channel unit as a proxy (T vs T/m).
                // Use unit: FIFF_UNIT_T = 112, FIFF_UNIT_T_M = 201
                if (fiffInfo.chs[i].unit == 112) {  // Tesla — magnetometer
                    magIdx.append(i);
                }
            }
        }

        if (magIdx.isEmpty()) {
            // Fall back: any MEG channel
            for (int i = 0; i < fiffInfo.nchan; ++i) {
                if (fiffInfo.chs[i].kind == FIFFV_MEG_CH) {
                    magIdx.append(i);
                }
            }
        }

        if (magIdx.isEmpty()) {
            qWarning() << "ArtifactDetect::detectEcg: No MEG channels found. Cannot detect ECG.";
            return {};
        }

        const int nSamp = static_cast<int>(matData.cols());
        ecgSignal = RowVectorXd::Zero(nSamp);
        for (int idx : magIdx) {
            ecgSignal += matData.row(idx).cwiseAbs().cast<double>();
        }
        ecgSignal /= static_cast<double>(magIdx.size());
    }

    // ---- Band-pass filter to isolate QRS complex ----
    RowVectorXd filtered = bandpassFilter(ecgSignal, dSFreq,
                                           params.dFilterLow, params.dFilterHigh,
                                           params.iFilterOrder);

    // ---- Adaptive threshold: fraction of the peak-to-peak amplitude ----
    double sigMin = filtered.minCoeff();
    double sigMax = filtered.maxCoeff();
    double threshold = params.dThreshFactor * (sigMax - sigMin) + sigMin;

    // Minimum inter-peak distance in samples
    int iMinDist = static_cast<int>(std::round(params.dMinRRSec * dSFreq));
    iMinDist = std::max(iMinDist, 1);

    return findPeaks(filtered, threshold, iMinDist);
}

//=============================================================================================================

QVector<int> ArtifactDetect::detectEog(const MatrixXd&  matData,
                                         const FiffInfo&  fiffInfo,
                                         double           dSFreq,
                                         const EogParams& params)
{
    // ---- Find EOG channel(s) ----
    QVector<int> eogIdx;
    for (int i = 0; i < fiffInfo.nchan; ++i) {
        if (fiffInfo.chs[i].kind == FIFFV_EOG_CH) {
            eogIdx.append(i);
        }
    }

    if (eogIdx.isEmpty()) {
        qWarning() << "ArtifactDetect::detectEog: No EOG channel found.";
        return {};
    }

    // Select the EOG channel with the largest peak-to-peak amplitude
    int bestIdx = eogIdx[0];
    double bestPtp = 0.0;
    for (int idx : eogIdx) {
        RowVectorXd row = matData.row(idx).cast<double>();
        double ptp = row.maxCoeff() - row.minCoeff();
        if (ptp > bestPtp) {
            bestPtp = ptp;
            bestIdx = idx;
        }
    }

    RowVectorXd eogSignal = matData.row(bestIdx).cast<double>();

    // ---- Low-pass filter ----
    RowVectorXd filtered = bandpassFilter(eogSignal, dSFreq,
                                           0.0, params.dFilterHigh,
                                           params.iFilterOrder);

    // ---- Detect excursions above ±threshold ----
    // Work on the absolute value so both positive (downward blinks, depending on EOG polarity)
    // and negative deflections are detected.
    RowVectorXd absFiltered = filtered.cwiseAbs();

    int iMinDist = static_cast<int>(std::round(params.dMinGapSec * dSFreq));
    iMinDist = std::max(iMinDist, 1);

    return findPeaks(absFiltered, params.dThresholdV, iMinDist);
}
