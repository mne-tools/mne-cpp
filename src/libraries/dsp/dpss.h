//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file dpss.h
 * @since 2026
 * @date  April 2026
 * @brief Discrete Prolate Spheroidal Sequences (Slepian tapers) for multitaper spectral estimation.
 *
 * The DPSS family is the unique set of sequences that maximise the energy
 * concentration of a length-@c N signal inside a normalised half-bandwidth
 * @c W. The first @c K ≈ 2NW eigenvectors of the corresponding
 * Toeplitz-sinc kernel form an approximately uncorrelated orthonormal
 * taper basis (Slepian, 1978; Thomson, 1982); their direct eigen-
 * concentrations λ_k determine the leakage of every taper.
 *
 * Used by @ref MultitaperPsd and @ref MultitaperTfr to suppress spectral
 * bias while keeping the variance penalty of windowing under control. The
 * implementation diagonalises the tri-diagonal Slepian matrix — numerically
 * far better-behaved than working with the sinc kernel directly — and
 * returns both the tapers and their concentrations so callers can apply
 * adaptive (eigen-weighted) combining.
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
