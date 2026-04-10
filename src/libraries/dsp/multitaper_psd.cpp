//=============================================================================================================
/**
 * @file     multitaper_psd.cpp
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
 * @brief    Implementation of MultitaperPsd.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "multitaper_psd.h"
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

MultitaperPsdResult MultitaperPsd::compute(const MatrixXd& matData,
                                            double          sfreq,
                                            double          halfBandwidth,
                                            int             nTapers,
                                            int             nFft)
{
    const int nChannels = static_cast<int>(matData.rows());
    const int nTimes    = static_cast<int>(matData.cols());

    if (nFft < 0)
        nFft = nTimes;

    const int nFreqs = nFft / 2 + 1;

    // 1. Compute DPSS tapers
    DpssResult dpss = Dpss::compute(nTimes, halfBandwidth, nTapers);
    const int nTap = static_cast<int>(dpss.matTapers.rows());

    // Compute eigenvalue weights for weighted averaging
    double weightSum = 0.0;
    for (int t = 0; t < nTap; ++t)
        weightSum += dpss.vecEigenvalues[t];

    MultitaperPsdResult result;
    result.matPsd = MatrixXd::Zero(nChannels, nFreqs);

    // Build frequency axis
    result.vecFreqs.resize(nFreqs);
    for (int k = 0; k < nFreqs; ++k)
        result.vecFreqs[k] = static_cast<double>(k) * sfreq / static_cast<double>(nFft);

    Eigen::FFT<double> fft;

    // 2. For each channel, apply each taper, FFT, accumulate weighted |X|^2
    for (int ch = 0; ch < nChannels; ++ch) {
        RowVectorXd psd = RowVectorXd::Zero(nFreqs);

        for (int t = 0; t < nTap; ++t) {
            // Apply taper element-wise
            VectorXd tapered = matData.row(ch).transpose().array()
                             * dpss.matTapers.row(t).transpose().array();

            // Zero-pad to nFft if needed
            VectorXd padded = VectorXd::Zero(nFft);
            const int copyLen = std::min(nTimes, nFft);
            padded.head(copyLen) = tapered.head(copyLen);

            // Forward FFT
            VectorXcd spec;
            fft.fwd(spec, padded);

            // Accumulate weighted |X[k]|^2
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

        result.matPsd.row(ch) = psd;
    }

    return result;
}
