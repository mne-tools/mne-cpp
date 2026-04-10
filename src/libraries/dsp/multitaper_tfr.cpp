//=============================================================================================================
/**
 * @file     multitaper_tfr.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.0
 * @date     April, 2026
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
 * @brief    Implementation of MultitaperTfr.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multitaper_tfr.h"
#include "dpss.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
//#ifndef EIGEN_FFTW_DEFAULT
//#define EIGEN_FFTW_DEFAULT
//#endif
#include <unsupported/Eigen/FFT>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MultitaperTfrResult MultitaperTfr::compute(const MatrixXd& matData,
                                            double          sfreq,
                                            int             windowSize,
                                            int             stepSize,
                                            double          halfBandwidth,
                                            int             nTapers)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes    = static_cast<int>(matData.cols());

    if (stepSize < 0)
        stepSize = windowSize / 2;

    const int nFreqs = windowSize / 2 + 1;

    // 1. Compute DPSS tapers for the window size
    DpssResult dpss = Dpss::compute(windowSize, halfBandwidth, nTapers);
    const int nTap = static_cast<int>(dpss.matTapers.rows());

    // Compute eigenvalue weight sum
    double weightSum = 0.0;
    for (int t = 0; t < nTap; ++t)
        weightSum += dpss.vecEigenvalues[t];

    // 2. Determine number of time steps and build time/freq axes
    int nSteps = 0;
    for (int start = 0; start + windowSize <= nTimes; start += stepSize)
        ++nSteps;

    MultitaperTfrResult result;

    // Frequency axis
    result.vecFreqs.resize(nFreqs);
    for (int k = 0; k < nFreqs; ++k)
        result.vecFreqs[k] = static_cast<double>(k) * sfreq / static_cast<double>(windowSize);

    // Time axis: centre of each window in seconds
    result.vecTimes.resize(nSteps);
    for (int s = 0; s < nSteps; ++s) {
        const int start = s * stepSize;
        const double centre = static_cast<double>(start) + static_cast<double>(windowSize - 1) / 2.0;
        result.vecTimes[s] = centre / sfreq;
    }

    // 3. Compute TFR for each channel
    Eigen::FFT<double> fft;

    result.tfrData.reserve(nChannels);
    for (int ch = 0; ch < nChannels; ++ch) {
        MatrixXd tfr = MatrixXd::Zero(nFreqs, nSteps);

        int stepIdx = 0;
        for (int start = 0; start + windowSize <= nTimes; start += stepSize) {
            // Extract window segment
            VectorXd segment = matData.row(ch).segment(start, windowSize).transpose();

            // Apply each taper, FFT, accumulate weighted |X|^2
            RowVectorXd psd = RowVectorXd::Zero(nFreqs);

            for (int t = 0; t < nTap; ++t) {
                VectorXd tapered = segment.array()
                                 * dpss.matTapers.row(t).transpose().array();

                VectorXcd spec;
                fft.fwd(spec, tapered);

                const double w = dpss.vecEigenvalues[t];
                for (int k = 0; k < nFreqs; ++k)
                    psd[k] += w * std::norm(spec[k]);
            }

            // Normalise: divide by (weightSum × sfreq)
            const double dNorm = 1.0 / (weightSum * sfreq);
            psd *= dNorm;

            // One-sided: double all non-DC, non-Nyquist bins
            for (int k = 1; k < nFreqs - 1; ++k)
                psd[k] *= 2.0;

            tfr.col(stepIdx) = psd.transpose();
            ++stepIdx;
        }

        result.tfrData.append(tfr);
    }

    return result;
}
