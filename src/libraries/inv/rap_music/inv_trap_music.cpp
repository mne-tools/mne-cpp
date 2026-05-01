//=============================================================================================================
/**
 * @file     inv_trap_music.cpp
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
 * @brief    InvTrapMusic class implementation.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_trap_music.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/SVD>
#include <Eigen/Dense>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// STD INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace INVLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvTrapMusic::InvTrapMusic(int iMaxSources, double dThreshold)
: m_iMaxSources(iMaxSources)
, m_dThreshold(dThreshold)
{
}

//=============================================================================================================

QList<TrapMusicDipole> InvTrapMusic::compute(const MatrixXd& matLeadField,
                                              const MatrixXd& matData,
                                              const MatrixXd& matSourcePos,
                                              int iNOrient) const
{
    QList<TrapMusicDipole> dipoles;

    const int nCh = static_cast<int>(matLeadField.rows());
    const int nSrc = static_cast<int>(matSourcePos.rows());

    if (nCh == 0 || matData.rows() != nCh || matLeadField.cols() != nSrc * iNOrient) {
        qWarning() << "[InvTrapMusic::compute] Dimension mismatch.";
        return dipoles;
    }

    // Estimate signal subspace dimension from SVD of data
    JacobiSVD<MatrixXd> dataSvd(matData, ComputeThinU);
    const VectorXd& singVals = dataSvd.singularValues();

    // Determine signal subspace dimension: look for a significant gap in singular values
    int nSignal = 1;
    for (int i = 1; i < singVals.size(); ++i) {
        if (singVals[i] < singVals[0] * 0.05)  // Below 5% of max
            break;
        ++nSignal;
    }
    nSignal = std::max(nSignal, m_iMaxSources);
    nSignal = std::min(nSignal, static_cast<int>(std::min(dataSvd.matrixU().cols(),
                                                           static_cast<Index>(nCh / 2))));

    // Signal subspace: U_s columns
    MatrixXd signalSubspace = dataSvd.matrixU().leftCols(nSignal);

    // Projector for the found sources (starts as identity)
    MatrixXd projector = MatrixXd::Identity(nCh, nCh);

    for (int iter = 0; iter < m_iMaxSources; ++iter) {
        // Project the lead field and signal subspace
        MatrixXd projLF = projector * matLeadField;
        MatrixXd projSS = projector * signalSubspace;

        // Re-orthogonalise the projected signal subspace
        int ssDim = static_cast<int>(projSS.cols()) - iter;
        if (ssDim < 1)
            break;
        ssDim = std::min(ssDim, static_cast<int>(projSS.cols()));

        JacobiSVD<MatrixXd> ssSvd(projSS, ComputeThinU);
        MatrixXd truncSS = ssSvd.matrixU().leftCols(ssDim);  // Truncation step

        // Scan correlations
        VectorXd correlations = scanCorrelations(projLF, truncSS, iNOrient);

        // Find best source
        Index bestIdx = 0;
        double bestCorr = correlations.maxCoeff(&bestIdx);

        if (bestCorr < m_dThreshold)
            break;

        // Build dipole result
        TrapMusicDipole dipole;
        dipole.sourceIdx = static_cast<int>(bestIdx);
        dipole.correlation = bestCorr;
        dipole.position = matSourcePos.row(bestIdx).transpose();

        // Estimate orientation from lead field columns
        int colStart = static_cast<int>(bestIdx) * iNOrient;
        if (iNOrient == 1) {
            dipole.orientation = Vector3d(0, 0, 1);
        } else {
            // Project lead field columns onto signal subspace to get orientation
            MatrixXd lfSrc = matLeadField.block(0, colStart, nCh, iNOrient);
            MatrixXd proj = truncSS * truncSS.transpose() * lfSrc;
            JacobiSVD<MatrixXd> orientSvd(proj, ComputeThinV);
            Vector3d orient = orientSvd.matrixV().col(0);
            orient.normalize();
            dipole.orientation = orient;
        }

        dipoles.append(dipole);

        // Project out the found source from the lead field (RAP step)
        MatrixXd lfSrc = matLeadField.block(0, colStart, nCh, iNOrient);
        MatrixXd lfOrth = lfSrc.householderQr().householderQ() *
                          MatrixXd::Identity(nCh, iNOrient);
        projector = projector - lfOrth * lfOrth.transpose() * projector;
    }

    return dipoles;
}

//=============================================================================================================

VectorXd InvTrapMusic::scanCorrelations(const MatrixXd& matLeadField,
                                          const MatrixXd& matSignalSubspace,
                                          int iNOrient)
{
    const int nSrcTotal = static_cast<int>(matLeadField.cols()) / iNOrient;
    VectorXd correlations(nSrcTotal);

    // Projector onto signal subspace
    MatrixXd P_s = matSignalSubspace * matSignalSubspace.transpose();

    for (int s = 0; s < nSrcTotal; ++s) {
        int colStart = s * iNOrient;
        MatrixXd G_s = matLeadField.block(0, colStart, matLeadField.rows(), iNOrient);

        // MUSIC correlation: ||P_s * G_s||_F / ||G_s||_F
        MatrixXd projG = P_s * G_s;
        double normProjG = projG.norm();
        double normG = G_s.norm();

        correlations[s] = (normG > 1e-15) ? (normProjG / normG) : 0.0;
    }

    return correlations;
}
