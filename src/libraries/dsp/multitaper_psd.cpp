//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file multitaper_psd.cpp
 * @since 2026
 * @date  April 2026
 * @brief Implementation of MultitaperPsd.
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
