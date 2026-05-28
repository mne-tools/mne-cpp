//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     inv_trap_music.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.2.1
 * @date     May 2026
 * @brief    Implementation of the TRAP-MUSIC scanning algorithm.
 *
 * Implements the SVD-based signal-subspace estimator, the per-grid-
 * point @c scanCorrelations helper that evaluates the maximum
 * subspace correlation for fixed or free orientation, the iterative
 * sub-space projection + truncation loop and the dipole-record
 * assembly. Operations are vectorised over the grid so the scan
 * remains tractable on dense source spaces.
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
