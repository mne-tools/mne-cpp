//=============================================================================================================
/**
 * @file     dpss.h
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
 * @brief    Dpss class declaration — Discrete Prolate Spheroidal Sequences (Slepian tapers).
 *
 */

#ifndef DPSS_H
#define DPSS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "dsp_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * @brief Result of a DPSS taper computation.
 */
struct DSPSHARED_EXPORT DpssResult
{
    Eigen::MatrixXd matTapers;      ///< nTapers × N, each row is a unit-norm taper
    Eigen::VectorXd vecEigenvalues; ///< Concentration ratios, length nTapers
};

//=============================================================================================================
/**
 * @brief Discrete Prolate Spheroidal Sequences (Slepian tapers).
 *
 * Computes DPSS tapers via the tridiagonal eigenvalue formulation. These tapers
 * are used by the multitaper spectral estimator to achieve optimal spectral
 * concentration within a given half-bandwidth.
 *
 * @code
 *   // 256-sample window, half-bandwidth 4, default number of tapers (7)
 *   DpssResult r = Dpss::compute(256, 4.0);
 *   // r.matTapers   → 7 × 256
 *   // r.vecEigenvalues → 7 concentration ratios ≈ 1.0
 * @endcode
 */
class DSPSHARED_EXPORT Dpss
{
public:
    //=========================================================================================================
    /**
     * Compute DPSS tapers for a given window length and half-bandwidth.
     *
     * @param[in] N               Window length in samples.
     * @param[in] halfBandwidth   Half-bandwidth parameter (NW); typical values: 2, 3, 4.
     * @param[in] nTapers         Number of tapers to return; -1 → floor(2*halfBandwidth - 1).
     * @return                    DpssResult with matTapers (nTapers × N) and vecEigenvalues.
     */
    static DpssResult compute(int N, double halfBandwidth, int nTapers = -1);
};

} // namespace UTILSLIB

#endif // DPSS_H
