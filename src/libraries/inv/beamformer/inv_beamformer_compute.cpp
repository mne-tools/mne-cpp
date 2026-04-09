//=============================================================================================================
/**
 * @file     inv_beamformer_compute.cpp
 * @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    2.1.0
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
 * @brief    Definition of the InvBeamformerCompute class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_beamformer_compute.h"

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

#include <cmath>
#include <algorithm>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace INVLIB;

//=============================================================================================================
// STATIC HELPERS
//=============================================================================================================

namespace {

/**
 * Invert a small symmetric positive-definite matrix using eigendecomposition.
 * Handles 1x1 (scalar) and 3x3 (free orientation) cases.
 */
MatrixXd invertSmallSym(const MatrixXd &X, bool reduceRank)
{
    const int n = static_cast<int>(X.rows());
    if(n == 1) {
        MatrixXd result(1, 1);
        double val = X(0, 0);
        result(0, 0) = (std::abs(val) > 1e-30) ? 1.0 / val : 1.0;
        return result;
    }
    return InvBeamformerCompute::symMatPow(X, -1.0, reduceRank);
}

} // anonymous namespace

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

void InvBeamformerCompute::regPinv(const MatrixXd &C,
                                   double reg,
                                   MatrixXd &CInv,
                                   double &loadingFactor,
                                   int &rankOut)
{
    const int n = static_cast<int>(C.rows());

    // Eigendecomposition of symmetric matrix
    SelfAdjointEigenSolver<MatrixXd> eig(C);
    VectorXd eigVals = eig.eigenvalues();   // ascending order
    MatrixXd eigVecs = eig.eigenvectors();

    // Determine rank: count eigenvalues above threshold
    double maxEig = eigVals.maxCoeff();
    double threshold = maxEig * 1e-10;
    rankOut = 0;
    for(int i = 0; i < n; ++i) {
        if(eigVals(i) > threshold)
            ++rankOut;
    }

    if(rankOut == 0) {
        qWarning("InvBeamformerCompute::regPinv - Covariance matrix has zero rank!");
        CInv = MatrixXd::Zero(n, n);
        loadingFactor = 0.0;
        return;
    }

    // Loading factor: reg * trace / rank
    double trace = eigVals.sum();
    loadingFactor = reg * trace / static_cast<double>(rankOut);

    // Regularize: lambda_i += loading_factor (for significant eigenvalues)
    // Then invert: 1 / (lambda_i + loading)
    VectorXd eigValsInv(n);
    for(int i = 0; i < n; ++i) {
        if(eigVals(i) > threshold) {
            eigValsInv(i) = 1.0 / (eigVals(i) + loadingFactor);
        } else {
            eigValsInv(i) = 0.0;
        }
    }

    // Reconstruct inverse: V diag(1/lambda) V^T
    CInv = eigVecs * eigValsInv.asDiagonal() * eigVecs.transpose();
}

//=============================================================================================================

void InvBeamformerCompute::reduceLeadfieldRank(MatrixXd &Gk)
{
    // SVD of per-source leadfield: Gk (n_channels, n_orient)
    JacobiSVD<MatrixXd> svd(Gk, ComputeThinU | ComputeThinV);
    MatrixXd U = svd.matrixU();
    VectorXd S = svd.singularValues();
    MatrixXd V = svd.matrixV();

    // Drop the smallest singular value
    const int keep = static_cast<int>(S.size()) - 1;
    if(keep <= 0) return;

    // Reconstruct without smallest component
    Gk = U.leftCols(keep) * S.head(keep).asDiagonal() * V.leftCols(keep).transpose();
}

//=============================================================================================================

MatrixXd InvBeamformerCompute::symMatPow(const MatrixXd &X, double p, bool reduceRank)
{
    const int n = static_cast<int>(X.rows());
    SelfAdjointEigenSolver<MatrixXd> eig(X);
    VectorXd eigVals = eig.eigenvalues();
    MatrixXd eigVecs = eig.eigenvectors();

    // Find threshold
    double maxEig = eigVals.cwiseAbs().maxCoeff();
    double threshold = maxEig * 1e-10;

    // Determine how many to keep
    int startIdx = 0;
    if(reduceRank) {
        // Find first (smallest magnitude) eigenvalue above threshold, then skip it
        for(int i = 0; i < n; ++i) {
            if(std::abs(eigVals(i)) > threshold) {
                startIdx = i + 1; // skip this one
                break;
            }
        }
    }

    // Compute: V diag(lambda^p) V^T for significant eigenvalues
    VectorXd eigPow = VectorXd::Zero(n);
    for(int i = startIdx; i < n; ++i) {
        if(std::abs(eigVals(i)) > threshold) {
            eigPow(i) = std::pow(eigVals(i), p);
        }
    }

    return eigVecs * eigPow.asDiagonal() * eigVecs.transpose();
}

//=============================================================================================================

bool InvBeamformerCompute::computeBeamformer(const MatrixXd &G,
                                             const MatrixXd &Cm,
                                             double reg,
                                             int nOrient,
                                             BeamformerWeightNorm weightNorm,
                                             BeamformerPickOri pickOri,
                                             bool reduceRank,
                                             BeamformerInversion invMethod,
                                             const MatrixX3d &nn,
                                             MatrixXd &W,
                                             MatrixX3d &maxPowerOri)
{
    const int nChannels = static_cast<int>(G.rows());
    const int nDipoles = static_cast<int>(G.cols());
    const int nSources = nDipoles / nOrient;

    if(nSources * nOrient != nDipoles) {
        qWarning("InvBeamformerCompute::computeBeamformer - G.cols() not divisible by nOrient!");
        return false;
    }
    if(Cm.rows() != nChannels || Cm.cols() != nChannels) {
        qWarning("InvBeamformerCompute::computeBeamformer - Cm dimension mismatch with leadfield!");
        return false;
    }

    // -----------------------------------------------------------------------
    // Step 1: Regularized pseudo-inverse of covariance
    // -----------------------------------------------------------------------
    MatrixXd CmInv;
    double loadingFactor = 0.0;
    int cmRank = 0;
    regPinv(Cm, reg, CmInv, loadingFactor, cmRank);

    // -----------------------------------------------------------------------
    // Step 2--6: Per-source computation
    // -----------------------------------------------------------------------
    // Determine output orientation count
    int nOrientOut = nOrient;
    if(pickOri == BeamformerPickOri::Normal || pickOri == BeamformerPickOri::MaxPower) {
        nOrientOut = 1;
    }
    // For Vector mode, keep all 3
    if(pickOri == BeamformerPickOri::Vector) {
        nOrientOut = nOrient;
    }

    W.resize(nSources * nOrientOut, nChannels);
    W.setZero();

    if(pickOri == BeamformerPickOri::MaxPower) {
        maxPowerOri.resize(nSources, 3);
        maxPowerOri.setZero();
    } else {
        maxPowerOri.resize(0, 3);
    }

    for(int s = 0; s < nSources; ++s) {
        // Extract per-source leadfield block: Gk (n_channels, n_orient)
        MatrixXd Gk = G.middleCols(s * nOrient, nOrient);

        // Step 3: Optional rank reduction
        if(reduceRank && nOrient > 1) {
            reduceLeadfieldRank(Gk);
        }

        // ------------------------------------------------------------------
        // Step 4: Orientation selection
        // ------------------------------------------------------------------
        int orientForFilter = nOrient;

        if(pickOri == BeamformerPickOri::MaxPower) {
            // Compute optimal orientation via max eigenvalue criterion
            // bf_numer = Gk^T Cm^{-1}   (n_orient, n_channels)
            // bf_denom = Gk^T Cm^{-1} Gk (n_orient, n_orient)
            MatrixXd bfNumer = Gk.transpose() * CmInv;               // (n_orient, n_ch)
            MatrixXd bfDenom = bfNumer * Gk;                         // (n_orient, n_orient)

            MatrixXd oriNumer, oriDenom;
            if(weightNorm == BeamformerWeightNorm::None) {
                oriNumer = MatrixXd::Identity(nOrient, nOrient);
                oriDenom = bfDenom;
            } else {
                // Sekihara & Nagarajan 2008, eq. 4.47
                oriNumer = bfDenom;
                oriDenom = Gk.transpose() * (CmInv * CmInv) * Gk;   // (n_orient, n_orient)
            }

            // Compute oriDenom^{-1} @ oriNumer
            MatrixXd oriDenomInv = invertSmallSym(oriDenom, reduceRank);
            MatrixXd oriPick = oriDenomInv * oriNumer;

            // Pick eigenvector with maximum absolute eigenvalue
            // Note: oriPick is NOT necessarily symmetric -> use general eigensolver
            EigenSolver<MatrixXd> eigSolve(oriPick);
            VectorXcd eigVals = eigSolve.eigenvalues();
            MatrixXcd eigVecs = eigSolve.eigenvectors();

            int maxIdx = 0;
            double maxVal = 0.0;
            for(int i = 0; i < eigVals.size(); ++i) {
                double absVal = std::abs(eigVals(i));
                if(absVal > maxVal) {
                    maxVal = absVal;
                    maxIdx = i;
                }
            }

            // Optimal orientation (real part)
            Vector3d ori = eigVecs.col(maxIdx).real().head(3).normalized();

            // Align sign with surface normal
            if(nn.rows() > s) {
                double dot = ori.dot(nn.row(s).transpose());
                if(dot < 0.0) ori = -ori;
            }

            maxPowerOri.row(s) = ori.transpose();

            // Project leadfield to optimal orientation
            Gk = Gk * ori;  // (n_channels, 1)
            orientForFilter = 1;

        } else if(pickOri == BeamformerPickOri::Normal && nOrient >= 3) {
            // Extract Z-component (normal to surface in local source coords)
            Gk = Gk.col(2);  // (n_channels, 1)
            orientForFilter = 1;
        }

        // ------------------------------------------------------------------
        // Step 5: Compute unit-gain filter
        //   bf_numer = Gk^T Cm^{-1}          (n_ori_filt, n_channels)
        //   bf_denom = Gk^T Cm^{-1} Gk       (n_ori_filt, n_ori_filt)
        //   W_ug     = bf_denom^{-1} bf_numer (n_ori_filt, n_channels)
        // ------------------------------------------------------------------
        MatrixXd bfNumer = Gk.transpose() * CmInv;    // (orientForFilter, n_ch)
        MatrixXd bfDenom = bfNumer * Gk;               // (orientForFilter, orientForFilter)

        MatrixXd bfDenomInv;
        if(invMethod == BeamformerInversion::Single && orientForFilter > 1) {
            // Scalar inversion of diagonal elements
            bfDenomInv = MatrixXd::Zero(orientForFilter, orientForFilter);
            for(int d = 0; d < orientForFilter; ++d) {
                double val = bfDenom(d, d);
                bfDenomInv(d, d) = (std::abs(val) > 1e-30) ? 1.0 / val : 0.0;
            }
        } else {
            bfDenomInv = invertSmallSym(bfDenom, reduceRank);
        }

        MatrixXd Wug = bfDenomInv * bfNumer;  // (orientForFilter, n_channels)

        // ------------------------------------------------------------------
        // Step 6: Weight normalization
        // ------------------------------------------------------------------
        if(weightNorm == BeamformerWeightNorm::UnitNoiseGain || weightNorm == BeamformerWeightNorm::NAI) {
            // Sekihara 2008: normalize by sqrt(diag(W W^T))
            MatrixXd noiseNorm = Wug * Wug.transpose();   // (orientForFilter, orientForFilter)

            for(int d = 0; d < orientForFilter; ++d) {
                double normVal = std::sqrt(std::abs(noiseNorm(d, d)));
                if(normVal > 1e-30) {
                    Wug.row(d) /= normVal;
                }
            }

            if(weightNorm == BeamformerWeightNorm::NAI) {
                // Additional normalization by noise level
                double noise = loadingFactor;
                if(noise > 1e-30) {
                    Wug /= std::sqrt(noise);
                }
            }

        } else if(weightNorm == BeamformerWeightNorm::UnitNoiseGainInv) {
            // Rotation-invariant version: sqrtm(inner)^{-0.5} @ G^T Cm^{-1}
            MatrixXd inner = bfNumer * bfNumer.transpose();   // (orientForFilter, orientForFilter)
            MatrixXd innerPow = symMatPow(inner, -0.5, reduceRank);
            Wug = innerPow * bfNumer;
        }

        // Store result
        W.middleRows(s * nOrientOut, nOrientOut) = Wug;
    }

    return true;
}

//=============================================================================================================

VectorXd InvBeamformerCompute::computePower(const MatrixXd &Cm,
                                            const MatrixXd &W,
                                            int nOrient)
{
    const int nTotal = static_cast<int>(W.rows());
    const int nSources = nTotal / nOrient;

    VectorXd power(nSources);
    for(int s = 0; s < nSources; ++s) {
        // W_k: (n_orient, n_channels)
        MatrixXd Wk = W.middleRows(s * nOrient, nOrient);
        // power = trace(W_k Cm W_k^T)
        MatrixXd WCW = Wk * Cm * Wk.transpose();
        power(s) = WCW.trace();
    }

    return power;
}
