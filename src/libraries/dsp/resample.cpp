//=============================================================================================================
/**
 * @file     resample.cpp
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
 * @brief    Implementation of Resample.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "resample.h"

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
using namespace Eigen;

//=============================================================================================================
// PRIVATE
//=============================================================================================================

int Resample::gcd(int a, int b)
{
    while (b) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

//=============================================================================================================

RowVectorXd Resample::buildKernel(int p, int q, int iNZeros)
{
    int M        = std::max(p, q);
    int halfLen  = iNZeros * M;
    int L        = 2 * halfLen + 1;

    // Cutoff as a fraction of the sampling frequency at the upsampled rate p*oldSFreq.
    // We want to cut at the lower Nyquist: min(oldSFreq, newSFreq)/2.
    // In normalised terms (fraction of upsampled fs):
    //   cutoff_norm = min(p,q) / (2.0 * max(p,q))  ... but the conventional sinc parameterisation
    //   uses cutoff as a fraction of fs (not Nyquist), i.e. in [0,0.5].
    // So fc_fs = min(p,q) / (2.0 * max(p,q)).
    const double fc = static_cast<double>(std::min(p, q)) / (2.0 * static_cast<double>(M));

    RowVectorXd h(L);
    for (int k = 0; k < L; ++k) {
        double n   = k - halfLen;
        double win = 0.54 - 0.46 * std::cos(2.0 * M_PI * k / (L - 1));  // Hamming

        if (std::abs(n) < 1e-10) {
            h(k) = 2.0 * fc * win;
        } else {
            // sinc(2*fc*n) * 2*fc  =  sin(2π*fc*n) / (π*n)
            h(k) = std::sin(2.0 * M_PI * fc * n) / (M_PI * n) * win;
        }
    }

    // Scale by p to restore unity gain after the conceptual upsampling-by-p step.
    h *= static_cast<double>(p);
    return h;
}

//=============================================================================================================

RowVectorXd Resample::polyphaseConv(const RowVectorXd& vecX,
                                     const RowVectorXd& vecH,
                                     int                p,
                                     int                q,
                                     int                halfLen)
{
    const long long nIn  = static_cast<long long>(vecX.size());
    const long long L    = static_cast<long long>(vecH.size());  // = 2*halfLen + 1

    // Output length: ceil(nIn * p / q)
    const long long nOut = (nIn * p + q - 1) / q;

    RowVectorXd y(nOut);

    for (long long m = 0; m < nOut; ++m) {
        // The filter delay (halfLen upsampled samples) is absorbed into the center index so that
        // output sample 0 aligns with input sample 0.
        long long center = m * static_cast<long long>(q) + halfLen;

        // Iterate over all input indices j whose upsampled position j*p lies within
        // the filter window [center - L + 1, center].
        // That is: j*p in [center - L + 1, center]
        //       => j in [ceil((center-L+1)/p), floor(center/p)]  ∩ [0, nIn-1]
        long long j_min = (center - L + 1 + p - 1) / p;  // ceil division
        if (j_min < 0) j_min = 0;
        long long j_max = center / p;
        if (j_max >= nIn) j_max = nIn - 1;

        double val = 0.0;
        for (long long j = j_min; j <= j_max; ++j) {
            long long tap = center - j * static_cast<long long>(p);
            if (tap >= 0 && tap < L) {
                val += vecH(static_cast<int>(tap)) * vecX(static_cast<int>(j));
            }
        }
        y(static_cast<int>(m)) = val;
    }

    return y;
}

//=============================================================================================================
// PUBLIC
//=============================================================================================================

RowVectorXd Resample::resample(const RowVectorXd& vecData,
                                double             dNewSFreq,
                                double             dOldSFreq,
                                int                iNZeros)
{
    if (vecData.size() == 0) {
        qWarning() << "Resample::resample: empty input.";
        return vecData;
    }
    if (dNewSFreq <= 0.0 || dOldSFreq <= 0.0) {
        qWarning() << "Resample::resample: sampling frequencies must be positive.";
        return vecData;
    }

    // Represent ratio as integers to avoid floating-point drift.
    // Multiply both rates by 1000 and reduce by GCD (handles e.g. 600.0/1000.0 → 3/5).
    const int scale = 1000;
    int p_raw = static_cast<int>(std::round(dNewSFreq * scale));
    int q_raw = static_cast<int>(std::round(dOldSFreq * scale));
    int g     = gcd(p_raw, q_raw);
    int p     = p_raw / g;
    int q     = q_raw / g;

    if (p == q) {
        return vecData;  // Same rate after reduction
    }

    const int halfLen = iNZeros * std::max(p, q);
    RowVectorXd h     = buildKernel(p, q, iNZeros);

    return polyphaseConv(vecData, h, p, q, halfLen);
}

//=============================================================================================================

MatrixXd Resample::resampleMatrix(const MatrixXd&    matData,
                                   double             dNewSFreq,
                                   double             dOldSFreq,
                                   const RowVectorXi& vecPicks,
                                   int                iNZeros)
{
    if (matData.size() == 0) return matData;

    // Pre-build kernel once for all channels
    const int scale = 1000;
    int p_raw = static_cast<int>(std::round(dNewSFreq * scale));
    int q_raw = static_cast<int>(std::round(dOldSFreq * scale));
    int g     = gcd(p_raw, q_raw);
    int p     = p_raw / g;
    int q     = q_raw / g;

    if (p == q) return matData;

    const int halfLen = iNZeros * std::max(p, q);
    RowVectorXd h     = buildKernel(p, q, iNZeros);

    const int nIn    = static_cast<int>(matData.cols());
    const int nOut   = static_cast<int>((static_cast<long long>(nIn) * p + q - 1) / q);
    const int nCh    = static_cast<int>(matData.rows());

    MatrixXd result(nCh, nOut);

    if (vecPicks.size() == 0) {
        for (int i = 0; i < nCh; ++i) {
            result.row(i) = polyphaseConv(matData.row(i), h, p, q, halfLen);
        }
    } else {
        // Initialise to zero then fill picked rows (non-picked rows remain zero —
        // callers using picks should be aware rows are not copied; use full resampling
        // if a copy of non-MEG channels is needed).
        result.setZero();
        for (int k = 0; k < vecPicks.size(); ++k) {
            int i = vecPicks(k);
            if (i >= 0 && i < nCh) {
                result.row(i) = polyphaseConv(matData.row(i), h, p, q, halfLen);
            }
        }
    }

    return result;
}
