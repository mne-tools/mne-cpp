//=============================================================================================================
/**
 * @file     inv_resolution_matrix.cpp
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
 * @brief    Implementation of InvResolutionMatrix.
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
