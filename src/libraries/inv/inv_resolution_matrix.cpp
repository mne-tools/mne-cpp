//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file inv_resolution_matrix.cpp
 * @since May 2026
 * @brief Implementation of the resolution-matrix analytics declared in @c inv_resolution_matrix.h.
 *
 * Implements the @c K·L product, per-source PSF / CTF extraction,
 * distance-weighted spatial spread and peak-localisation error against
 * a 3-D position table. All operations are dense Eigen matmuls — no
 * iteration is needed because the resolution matrix is computed once
 * per inverse operator and re-used to characterise every source point.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_resolution_matrix.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

MatrixXd InvResolutionMatrix::compute(const MatrixXd& matInverseKernel,
                                      const MatrixXd& matLeadField)
{
    if (matInverseKernel.cols() != matLeadField.rows()) {
        qWarning() << "[InvResolutionMatrix::compute] Dimension mismatch:"
                    << "inverse kernel cols" << matInverseKernel.cols()
                    << "!= lead field rows" << matLeadField.rows();
        return MatrixXd();
    }

    return matInverseKernel * matLeadField;
}

//=============================================================================================================

VectorXd InvResolutionMatrix::getPsf(const MatrixXd& matResolution, int iSourceIdx)
{
    if (iSourceIdx < 0 || iSourceIdx >= matResolution.cols()) {
        qWarning() << "[InvResolutionMatrix::getPsf] Index out of range:" << iSourceIdx;
        return VectorXd();
    }

    return matResolution.col(iSourceIdx);
}

//=============================================================================================================

VectorXd InvResolutionMatrix::getCtf(const MatrixXd& matResolution, int iSourceIdx)
{
    if (iSourceIdx < 0 || iSourceIdx >= matResolution.rows()) {
        qWarning() << "[InvResolutionMatrix::getCtf] Index out of range:" << iSourceIdx;
        return VectorXd();
    }

    return matResolution.row(iSourceIdx).transpose();
}

//=============================================================================================================

MatrixXd InvResolutionMatrix::getPsfs(const MatrixXd& matResolution,
                                      const VectorXi& vecSourceIdx)
{
    const int nIdx = static_cast<int>(vecSourceIdx.size());
    MatrixXd result(matResolution.rows(), nIdx);

    for (int i = 0; i < nIdx; ++i) {
        if (vecSourceIdx[i] >= 0 && vecSourceIdx[i] < matResolution.cols())
            result.col(i) = matResolution.col(vecSourceIdx[i]);
        else
            result.col(i).setZero();
    }

    return result;
}

//=============================================================================================================

MatrixXd InvResolutionMatrix::getCtfs(const MatrixXd& matResolution,
                                      const VectorXi& vecSourceIdx)
{
    const int nIdx = static_cast<int>(vecSourceIdx.size());
    MatrixXd result(nIdx, matResolution.cols());

    for (int i = 0; i < nIdx; ++i) {
        if (vecSourceIdx[i] >= 0 && vecSourceIdx[i] < matResolution.rows())
            result.row(i) = matResolution.row(vecSourceIdx[i]);
        else
            result.row(i).setZero();
    }

    return result;
}

//=============================================================================================================

VectorXd InvResolutionMatrix::spatialSpread(const MatrixXd& matResolution,
                                            const MatrixX3d& matPositions)
{
    const int nSrc = static_cast<int>(matResolution.rows());
    VectorXd spread(nSrc);

    for (int s = 0; s < nSrc; ++s) {
        // PSF = column s (or row s for CTF-based; here we use PSF convention)
        VectorXd psf = matResolution.col(s).cwiseAbs2();
        double psfSum = psf.sum();
        if (psfSum < 1e-30) {
            spread[s] = 0.0;
            continue;
        }

        // Normalise to distribution
        psf /= psfSum;

        // Compute weighted mean distance from source s
        double meanDist = 0.0;
        double meanDist2 = 0.0;
        for (int j = 0; j < nSrc; ++j) {
            double dist = (matPositions.row(j) - matPositions.row(s)).norm();
            meanDist += psf[j] * dist;
            meanDist2 += psf[j] * dist * dist;
        }

        // Standard deviation of the distance distribution
        spread[s] = std::sqrt(std::max(0.0, meanDist2 - meanDist * meanDist));
    }

    return spread;
}

//=============================================================================================================

VectorXd InvResolutionMatrix::peakLocalisationError(const MatrixXd& matResolution,
                                                     const MatrixX3d& matPositions)
{
    const int nSrc = static_cast<int>(matResolution.rows());
    VectorXd ple(nSrc);

    for (int s = 0; s < nSrc; ++s) {
        // Find peak of PSF (column s)
        VectorXd psf = matResolution.col(s).cwiseAbs();
        Index peakIdx = 0;
        psf.maxCoeff(&peakIdx);

        ple[s] = (matPositions.row(peakIdx) - matPositions.row(s)).norm();
    }

    return ple;
}
