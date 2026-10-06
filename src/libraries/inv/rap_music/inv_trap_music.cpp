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

    if (nCh == 0 || matData.rows() != nCh || matLeadField.cols() != static_cast<Eigen::Index>(nSrc) * iNOrient) {
        qWarning() << "[InvTrapMusic::compute] Dimension mismatch.";
        return dipoles;
    }

    // Estimate signal subspace dimension from SVD of data
    JacobiSVD<MatrixXd> dataSvd(matData, ComputeThinU);
    const VectorXd& singVals = dataSvd.singularValues();

    // Determine signal subspace dimension: look for a significant gap in singular values
    int nSignal = 1;
    for (int i = 1; i < singVals.size(); ++i) {
        if (singVals[i] < singVals[0] * 0.05) // Below 5% of max
            break;
        ++nSignal;
    }
    nSignal = std::max(nSignal, m_iMaxSources);
    nSignal = std::min(nSignal, static_cast<int>(std::min(dataSvd.matrixU().cols(), static_cast<Index>(nCh / 2))));

    // Signal subspace: U_s columns
    MatrixXd signalSubspace = dataSvd.matrixU().leftCols(nSignal);

    // Topographies G_k o_k of the sources found so far; the RAP step projects out their span.
    MatrixXd found(nCh, 0);
    MatrixXd projector = MatrixXd::Identity(nCh, nCh);

    for (int iter = 0; iter < m_iMaxSources; ++iter) {
        MatrixXd projLF = projector * matLeadField;
        MatrixXd projSS = projector * signalSubspace;

        // Truncation step: iteration k keeps n - k + 1 dimensions of the projected signal subspace
        const int ssDim = static_cast<int>(projSS.cols()) - iter;
        if (ssDim < 1)
            break;
        JacobiSVD<MatrixXd> ssSvd(projSS, ComputeThinU);
        const MatrixXd truncSS = ssSvd.matrixU().leftCols(ssDim);

        const VectorXd correlations = scanCorrelations(projLF, truncSS, iNOrient);
        Index bestIdx = 0;
        const double bestCorr = correlations.maxCoeff(&bestIdx);
        if (bestCorr < m_dThreshold)
            break;

        TrapMusicDipole dipole;
        dipole.sourceIdx = static_cast<int>(bestIdx);
        dipole.correlation = bestCorr;
        dipole.position = matSourcePos.row(bestIdx).transpose();

        // The orientation maximises the correlation of the projected topography P G_k o with the subspace.
        const int colStart = static_cast<int>(bestIdx) * iNOrient;
        VectorXd orient = VectorXd::Ones(1);
        if (iNOrient > 1) {
            const MatrixXd projSrc = projLF.middleCols(colStart, iNOrient);
            // Maximise ||U^T P G o|| / ||P G o||, a generalised eigenproblem with the Gram matrix of P G.
            const MatrixXd gram = projSrc.transpose() * projSrc;
            const MatrixXd fit = projSrc.transpose() * truncSS * truncSS.transpose() * projSrc;
            GeneralizedSelfAdjointEigenSolver<MatrixXd> ges(fit, gram);
            orient = ges.eigenvectors().col(iNOrient - 1);
            orient.normalize();
            dipole.orientation = Vector3d(orient(0), orient(1), orient(2));
        } else {
            dipole.orientation = Vector3d(0, 0, 1);
        }
        dipoles.append(dipole);

        // RAP step: P = I - A A^+ with A the found topographies
        found.conservativeResize(NoChange, found.cols() + 1);
        found.col(found.cols() - 1) = matLeadField.middleCols(colStart, iNOrient) * orient;
        const MatrixXd q = found.householderQr().householderQ() * MatrixXd::Identity(nCh, found.cols());
        projector = MatrixXd::Identity(nCh, nCh) - q * q.transpose();
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

    for (int s = 0; s < nSrcTotal; ++s) {
        const MatrixXd G_s = matLeadField.middleCols(s * iNOrient, iNOrient);
        if (G_s.norm() <= 1e-15) {
            correlations[s] = 0.0;
            continue;
        }
        // Subspace correlation (Mosher & Leahy): largest singular value of U_s^T orth(G_s), the best
        // orientation's cosine with the signal subspace.
        JacobiSVD<MatrixXd> gSvd(G_s, ComputeThinU);
        const VectorXd& sv = gSvd.singularValues();
        int rank = 0;
        while (rank < sv.size() && sv(rank) > 1e-10 * sv(0))
            ++rank;
        const MatrixXd basis = gSvd.matrixU().leftCols(rank);
        correlations[s] = JacobiSVD<MatrixXd>(matSignalSubspace.transpose() * basis).singularValues()(0);
    }

    return correlations;
}
