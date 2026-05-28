//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_resolution_matrix.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Resolution-matrix analysis for linear inverse operators — point-spread and cross-talk functions.
 *
 * @ref INVLIB::InvResolutionMatrix computes
 * @f$ \mathbf{R} = \mathbf{K}\,\mathbf{L} @f$ from an inverse kernel
 * @c K and a leadfield @c L, then exposes the column-wise point-spread
 * functions (PSF), row-wise cross-talk functions (CTF), spatial-spread
 * and peak-localisation-error metrics that characterise the spatial
 * resolution of an inverse operator. The class follows the methodology
 * of Hauk et al. (NeuroImage 2011) — used in mne-python's
 * @c mne.minimum_norm.resolution_matrix module — so that resolution
 * diagnostics produced by mne-cpp are directly comparable to the
 * upstream Python results.
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
