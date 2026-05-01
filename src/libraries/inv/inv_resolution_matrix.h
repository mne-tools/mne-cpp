//=============================================================================================================
/**
 * @file     inv_resolution_matrix.h
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
 * @brief    InvResolutionMatrix class — resolution matrix, PSF, and CTF analysis.
 *
 */

#ifndef INV_RESOLUTION_MATRIX_H
#define INV_RESOLUTION_MATRIX_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE INVLIB
//=============================================================================================================

namespace INVLIB
{

//=============================================================================================================
/**
 * @brief Resolution matrix analysis for linear inverse operators.
 *
 * The resolution matrix R = K · L, where K is the inverse operator (kernel)
 * and L is the lead field (forward operator). Columns of R are point-spread
 * functions (PSFs) and rows are cross-talk functions (CTFs).
 *
 * @code
 *   // Given an inverse kernel K (n_sources × n_channels)
 *   // and a lead field L (n_channels × n_sources):
 *   Eigen::MatrixXd R = InvResolutionMatrix::compute(K, L);
 *   Eigen::VectorXd psf = InvResolutionMatrix::getPsf(R, sourceIdx);
 *   Eigen::VectorXd ctf = InvResolutionMatrix::getCtf(R, sourceIdx);
 * @endcode
 */
class INVSHARED_EXPORT InvResolutionMatrix
{
public:
    //=========================================================================================================
    /**
     * Compute the resolution matrix R = inverseKernel × leadField.
     *
     * @param[in] matInverseKernel   Inverse operator matrix (n_sources × n_channels).
     * @param[in] matLeadField       Forward operator / lead field (n_channels × n_sources).
     * @return                       Resolution matrix (n_sources × n_sources).
     */
    static Eigen::MatrixXd compute(const Eigen::MatrixXd& matInverseKernel,
                                   const Eigen::MatrixXd& matLeadField);

    //=========================================================================================================
    /**
     * Extract the point-spread function (PSF) for a given source index.
     * The PSF is the column of the resolution matrix at the source index.
     *
     * @param[in] matResolution   Resolution matrix (n_sources × n_sources).
     * @param[in] iSourceIdx      Source index.
     * @return                    PSF vector (n_sources).
     */
    static Eigen::VectorXd getPsf(const Eigen::MatrixXd& matResolution,
                                  int iSourceIdx);

    //=========================================================================================================
    /**
     * Extract the cross-talk function (CTF) for a given source index.
     * The CTF is the row of the resolution matrix at the source index.
     *
     * @param[in] matResolution   Resolution matrix (n_sources × n_sources).
     * @param[in] iSourceIdx      Source index.
     * @return                    CTF vector (n_sources).
     */
    static Eigen::VectorXd getCtf(const Eigen::MatrixXd& matResolution,
                                  int iSourceIdx);

    //=========================================================================================================
    /**
     * Extract PSFs for multiple source indices.
     *
     * @param[in] matResolution   Resolution matrix.
     * @param[in] vecSourceIdx    Source indices.
     * @return                    Matrix where each column is a PSF (n_sources × n_indices).
     */
    static Eigen::MatrixXd getPsfs(const Eigen::MatrixXd& matResolution,
                                   const Eigen::VectorXi& vecSourceIdx);

    //=========================================================================================================
    /**
     * Extract CTFs for multiple source indices.
     *
     * @param[in] matResolution   Resolution matrix.
     * @param[in] vecSourceIdx    Source indices.
     * @return                    Matrix where each row is a CTF (n_indices × n_sources).
     */
    static Eigen::MatrixXd getCtfs(const Eigen::MatrixXd& matResolution,
                                   const Eigen::VectorXi& vecSourceIdx);

    //=========================================================================================================
    /**
     * Compute spatial spread (peak-width) metric for each source.
     *
     * For each source, the spatial spread is the standard deviation of the
     * squared PSF weighted by distance from the peak.
     *
     * @param[in] matResolution   Resolution matrix (n_sources × n_sources).
     * @param[in] matPositions    Source positions (n_sources × 3).
     * @return                    Spatial spread per source (n_sources).
     */
    static Eigen::VectorXd spatialSpread(const Eigen::MatrixXd& matResolution,
                                         const Eigen::MatrixX3d& matPositions);

    //=========================================================================================================
    /**
     * Compute peak localisation error for each source.
     *
     * Distance between true source location and the location of the
     * maximum of the PSF.
     *
     * @param[in] matResolution   Resolution matrix (n_sources × n_sources).
     * @param[in] matPositions    Source positions (n_sources × 3).
     * @return                    Peak localisation error per source (n_sources).
     */
    static Eigen::VectorXd peakLocalisationError(const Eigen::MatrixXd& matResolution,
                                                  const Eigen::MatrixX3d& matPositions);
};

} // namespace INVLIB

#endif // INV_RESOLUTION_MATRIX_H
