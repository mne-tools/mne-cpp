//=============================================================================================================
/**
 * @file     sss.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
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
 * @brief    Implementation of the SSS class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sss.h"

//=============================================================================================================
// FIFF INCLUDES
//=============================================================================================================

#include <fiff/fiff_constants.h>
#include <fiff/fiff_ch_info.h>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Dense>
#include <Eigen/SVD>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// C++ INCLUDES
//=============================================================================================================

#include <cmath>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace FIFFLIB;
using namespace Eigen;

//=============================================================================================================
// CONSTANTS
//=============================================================================================================

static constexpr double SSS_PI = M_PI;

//=============================================================================================================
// PRIVATE HELPERS
//=============================================================================================================

namespace {

//=============================================================================================================
/**
 * @brief Regularised pseudoinverse via truncated SVD.
 *        Singular values below reg * sigma_max are treated as zero.
 */
MatrixXd regPinv(const MatrixXd& A, double reg = 1e-5)
{
    JacobiSVD<MatrixXd> svd(A, ComputeThinU | ComputeThinV);
    const VectorXd& sv   = svd.singularValues();
    double threshold     = reg * sv(0);
    VectorXd invSv       = sv;
    for (int i = 0; i < invSv.size(); ++i) {
        invSv(i) = (sv(i) > threshold) ? 1.0 / sv(i) : 0.0;
    }
    return svd.matrixV() * invSv.asDiagonal() * svd.matrixU().transpose();
}

} // anonymous namespace

//=============================================================================================================
// SSS MEMBER DEFINITIONS
//=============================================================================================================

void SSS::computeNormALP(int lmax, double cosTheta, double sinTheta,
                          MatrixXd& P, MatrixXd& dP)
{
    // Guard against pole singularity
    if (sinTheta < 1e-12) {
        sinTheta = 1e-12;
    }

    const int sz = lmax + 2;
    P.resize(sz, sz);
    dP.resize(sz, sz);
    P.setZero();
    dP.setZero();

    // ---- Build un-normalised ALPs using standard recurrence ----
    // P_0^0 = 1
    // P_1^0 = cos θ
    // P_1^1 = -sin θ
    // P_l^l = -(2l-1) * sin θ * P_{l-1}^{l-1}
    // P_l^{l-1} = (2l-1) * cos θ * P_{l-1}^{l-1}
    // P_l^m = [(2l-1)*cos θ * P_{l-1}^m - (l-1+m)*P_{l-2}^m] / (l-m)

    // Temporary table (un-normalised)
    MatrixXd Praw(sz, sz);
    Praw.setZero();
    Praw(0, 0) = 1.0;
    if (lmax >= 1) {
        Praw(1, 0) = cosTheta;
        Praw(1, 1) = -sinTheta;
    }
    for (int l = 2; l <= lmax + 1; ++l) {
        Praw(l, l) = -(2 * l - 1) * sinTheta * Praw(l - 1, l - 1);
        Praw(l, l - 1) = (2 * l - 1) * cosTheta * Praw(l - 1, l - 1);
        for (int m = 0; m <= l - 2; ++m) {
            Praw(l, m) = ((2 * l - 1) * cosTheta * Praw(l - 1, m)
                          - (l - 1 + m) * Praw(l - 2, m))
                         / static_cast<double>(l - m);
        }
    }

    // ---- Apply 4π-normalisation and compute dP/dθ ----
    for (int l = 1; l <= lmax; ++l) {
        for (int m = 0; m <= l; ++m) {
            // Normalisation factor N_l^m
            // (l-m)! / (l+m)!
            double fac = 1.0;
            for (int k = l - m + 1; k <= l + m; ++k) {
                fac /= static_cast<double>(k);
            }
            double norm = (m == 0)
                          ? std::sqrt((2.0 * l + 1.0) / (4.0 * SSS_PI) * fac)
                          : std::sqrt(2.0 * (2.0 * l + 1.0) / (4.0 * SSS_PI) * fac);

            P(l, m) = norm * Praw(l, m);

            // dP_l^m/dθ using:
            //   sin θ * dP_l^m/dθ = l * cos θ * P_l^m - (l+m) * P_{l-1}^m
            // (applied to un-normalised P, then multiplied by norm / sinθ)
            double sinThetaDeriv = static_cast<double>(l) * cosTheta * Praw(l, m);
            if (l - 1 >= m) {
                sinThetaDeriv -= static_cast<double>(l + m) * Praw(l - 1, m);
            }
            dP(l, m) = norm * sinThetaDeriv / sinTheta;
        }
    }
}

//=============================================================================================================

Vector3d SSS::basisGradCart(int l, int m, bool bInternal,
                              const Vector3d& rPos,
                              const MatrixXd& P, const MatrixXd& dP,
                              double cosTheta, double sinTheta,
                              double cosPhi,   double sinPhi)
{
    // Precompute |r| and its powers
    double r = rPos.norm();
    if (r < 1e-12) {
        return Vector3d::Zero();
    }

    const int absM = std::abs(m);

    // ---- Y_l^m and its derivatives ----
    //
    // Y_l^m (real):
    //   m  = 0 : N_l^0 * P_l^0(cos θ)
    //   m  > 0 : N_l^m * P_l^m(cos θ) * cos(m φ)
    //   m  < 0 : N_l^|m| * P_l^|m|(cos θ) * sin(|m| φ)
    //
    // Derivatives:
    //   dY/dθ:  N * dP/dθ * {1, cos(mφ), sin(|m|φ)}
    //   dY/dφ:  N * P * {0, -m sin(mφ), |m| cos(|m|φ)}

    double Plm   = P(l, absM);    // normalised, at (θ,φ)
    double dPlm  = dP(l, absM);   // normalised dP/dθ

    double angFactor, dAngFactor_theta, dAngFactor_phi;
    if (m == 0) {
        angFactor        = 1.0;
        dAngFactor_theta = 0.0;  // handled through dPlm
        dAngFactor_phi   = 0.0;
    } else if (m > 0) {
        double cosmPhi   = std::cos(static_cast<double>(m) * std::atan2(sinPhi, cosPhi));
        double sinmPhi   = std::sin(static_cast<double>(m) * std::atan2(sinPhi, cosPhi));
        angFactor        =  cosmPhi;
        dAngFactor_theta =  0.0;   // cos(mφ) doesn't depend on θ
        dAngFactor_phi   = -static_cast<double>(m) * sinmPhi;
    } else {
        // m < 0
        double cosmPhi   = std::cos(static_cast<double>(absM) * std::atan2(sinPhi, cosPhi));
        double sinmPhi   = std::sin(static_cast<double>(absM) * std::atan2(sinPhi, cosPhi));
        angFactor        =  sinmPhi;
        dAngFactor_theta =  0.0;
        dAngFactor_phi   =  static_cast<double>(absM) * cosmPhi;
    }

    // Y_l^m at this point
    double Ylm     = Plm * angFactor;

    // dY_l^m/dθ = dP/dθ * angFactor   (for m=0 angFactor=1, for m≠0 it's cos/sin but θ-independent)
    double dYdTheta = dPlm * angFactor;

    // dY_l^m/dφ  =  P_l^|m| * dAngFactor/dφ
    double dYdPhi  = Plm * dAngFactor_phi;

    // ---- Gradient in spherical coordinates ----
    // For internal:  grad(r^l * Y_l^m)
    //   G_r  = l  * r^{l-1} * Y_l^m
    //   G_θ  = r^{l-1} * dY/dθ
    //   G_φ  = r^{l-1} / sinθ * dY/dφ   (handle sinθ below)
    //
    // For external:  grad(r^{-(l+1)} * Y_l^m)
    //   G_r  = -(l+1) * r^{-(l+2)} * Y_l^m
    //   G_θ  = r^{-(l+2)} * dY/dθ
    //   G_φ  = r^{-(l+2)} / sinθ * dY/dφ

    double radPow, Gr_coeff, Gtu_coeff;
    if (bInternal) {
        radPow    = std::pow(r, l - 1);
        Gr_coeff  = static_cast<double>(l)   * radPow;
        Gtu_coeff =                             radPow;   // G_θ and sinθ*G_φ share this
    } else {
        radPow    = std::pow(r, -(l + 2));
        Gr_coeff  = -static_cast<double>(l + 1) * radPow;
        Gtu_coeff =                                radPow;
    }

    double Gr          = Gr_coeff  * Ylm;
    double Gtheta      = Gtu_coeff * dYdTheta;
    double GphiTimesSin = Gtu_coeff * dYdPhi;   // = G_φ * sinθ (avoids 1/sinθ singularity)

    // ---- Convert (Gr, Gθ, sinθ * Gφ) to Cartesian ----
    // r̂  = (sinθ cosφ,  sinθ sinφ,  cosθ)
    // θ̂  = (cosθ cosφ,  cosθ sinφ, -sinθ)
    // φ̂  = (-sinφ,      cosφ,       0   )
    //
    // grad_cart = Gr * r̂ + Gθ * θ̂ + Gφ * φ̂
    //           = Gr * r̂ + Gθ * θ̂ + (sinθ * Gφ) / sinθ * φ̂
    // Numerically we use GphiTimesSin / sinθ for the φ̂ component.

    double Gphi = (sinTheta > 1e-12) ? GphiTimesSin / sinTheta : 0.0;

    double gx = Gr * sinTheta * cosPhi + Gtheta * cosTheta * cosPhi - Gphi * sinPhi;
    double gy = Gr * sinTheta * sinPhi + Gtheta * cosTheta * sinPhi + Gphi * cosPhi;
    double gz = Gr * cosTheta           - Gtheta * sinTheta;

    return Vector3d(gx, gy, gz);
}

//=============================================================================================================

SSS::Basis SSS::computeBasis(const FiffInfo& fiffInfo, const Params& params)
{
    Basis basis;
    basis.iOrderIn  = params.iOrderIn;
    basis.iOrderOut = params.iOrderOut;
    basis.iNin      = params.iOrderIn  * (params.iOrderIn  + 2);
    basis.iNout     = params.iOrderOut * (params.iOrderOut + 2);

    // ---- Collect MEG channel indices and geometry ----
    for (int i = 0; i < fiffInfo.nchan; ++i) {
        int kind = fiffInfo.chs[i].kind;
        if (kind == FIFFV_MEG_CH || kind == FIFFV_REF_MEG_CH) {
            basis.megChannelIdx.append(i);
        }
    }

    const int nMeg = basis.megChannelIdx.size();
    if (nMeg == 0) {
        qWarning() << "SSS::computeBasis: no MEG channels found in FiffInfo.";
        return basis;
    }

    basis.matSin.resize(nMeg, basis.iNin);
    basis.matSout.resize(nMeg, basis.iNout);
    basis.matSin.setZero();
    basis.matSout.setZero();

    const int lmax = std::max(params.iOrderIn, params.iOrderOut);

    // ---- Fill basis matrices row-by-row (one sensor per row) ----
    for (int si = 0; si < nMeg; ++si) {
        const FiffChInfo& ch = fiffInfo.chs[basis.megChannelIdx[si]];

        // Sensor position relative to SSS origin (metres)
        Vector3d rPos(static_cast<double>(ch.chpos.r0(0)) - params.origin(0),
                      static_cast<double>(ch.chpos.r0(1)) - params.origin(1),
                      static_cast<double>(ch.chpos.r0(2)) - params.origin(2));

        // Sensor normal (ez of coil coordinate system)
        Vector3d normal(static_cast<double>(ch.chpos.ez(0)),
                        static_cast<double>(ch.chpos.ez(1)),
                        static_cast<double>(ch.chpos.ez(2)));

        // Normalise the normal vector
        double nNorm = normal.norm();
        if (nNorm < 1e-12) {
            continue;
        }
        normal /= nNorm;

        // Convert to spherical coordinates
        double r        = rPos.norm();
        if (r < 1e-12) {
            continue;
        }
        double cosTheta = rPos(2) / r;
        double sinTheta = std::sqrt(rPos(0) * rPos(0) + rPos(1) * rPos(1)) / r;
        if (sinTheta < 1e-12) sinTheta = 1e-12;
        double phi    = std::atan2(rPos(1), rPos(0));
        double cosPhi = std::cos(phi);
        double sinPhi = std::sin(phi);

        // Normalised ALP tables for this sensor
        MatrixXd P, dP;
        computeNormALP(lmax, cosTheta, sinTheta, P, dP);

        // Internal basis columns: iterate (l=1..L_in, m=-l..l)
        int colIn = 0;
        for (int l = 1; l <= params.iOrderIn; ++l) {
            for (int m = -l; m <= l; ++m) {
                Vector3d grad = basisGradCart(l, m, /*internal=*/true,
                                              rPos, P, dP,
                                              cosTheta, sinTheta, cosPhi, sinPhi);
                basis.matSin(si, colIn) = normal.dot(grad);
                ++colIn;
            }
        }

        // External basis columns: iterate (l=1..L_out, m=-l..l)
        int colOut = 0;
        for (int l = 1; l <= params.iOrderOut; ++l) {
            for (int m = -l; m <= l; ++m) {
                Vector3d grad = basisGradCart(l, m, /*internal=*/false,
                                              rPos, P, dP,
                                              cosTheta, sinTheta, cosPhi, sinPhi);
                basis.matSout(si, colOut) = normal.dot(grad);
                ++colOut;
            }
        }
    }

    // ---- Compute combined pseudoinverse and internal projector ----
    //
    // S = [S_in | S_out]  (n_meg × (N_in + N_out))
    // pinv(S) = V * diag(1/σ_i) * U^T  with Tikhonov regularisation
    // P_in = S_in * pinv(S)[:N_in, :]
    MatrixXd S(nMeg, basis.iNin + basis.iNout);
    S.leftCols(basis.iNin)  = basis.matSin;
    S.rightCols(basis.iNout) = basis.matSout;

    basis.matPinvAll = regPinv(S, params.dRegIn);          // (N_in+N_out) × n_meg
    basis.matProjIn  = basis.matSin * basis.matPinvAll.topRows(basis.iNin); // n_meg × n_meg

    return basis;
}

//=============================================================================================================

MatrixXd SSS::apply(const MatrixXd& matData, const Basis& basis)
{
    if (basis.megChannelIdx.isEmpty()) {
        return matData;
    }

    const int nMeg = basis.megChannelIdx.size();
    MatrixXd matOut = matData;

    // Extract MEG rows
    MatrixXd megData(nMeg, matData.cols());
    for (int i = 0; i < nMeg; ++i) {
        megData.row(i) = matData.row(basis.megChannelIdx[i]);
    }

    // Apply internal projector: data_sss = P_in * data_meg
    MatrixXd megSss = basis.matProjIn * megData;

    // Write back
    for (int i = 0; i < nMeg; ++i) {
        matOut.row(basis.megChannelIdx[i]) = megSss.row(i);
    }

    return matOut;
}

//=============================================================================================================

MatrixXd SSS::applyTemporal(const MatrixXd& matData,
                              const Basis&   basis,
                              int            iBufferLength,
                              double         dCorrLimit)
{
    if (basis.megChannelIdx.isEmpty()) {
        return matData;
    }

    const int nMeg    = basis.megChannelIdx.size();
    const int nSamp   = static_cast<int>(matData.cols());
    const int bufLen  = std::min(iBufferLength, nSamp);

    MatrixXd matOut = matData;

    // Extract MEG rows
    MatrixXd megData(nMeg, nSamp);
    for (int i = 0; i < nMeg; ++i) {
        megData.row(i) = matData.row(basis.megChannelIdx[i]);
    }

    // Decompose time series into expansion coefficients
    // c_in  (N_in  × nSamp) = pinv_all[:N_in , :] * megData
    // c_out (N_out × nSamp) = pinv_all[N_in: , :] * megData
    MatrixXd cIn  = basis.matPinvAll.topRows(basis.iNin)  * megData;   // N_in  × nSamp
    MatrixXd cOut = basis.matPinvAll.bottomRows(basis.iNout) * megData; // N_out × nSamp

    // Process in sliding windows
    int offset = 0;
    while (offset < nSamp) {
        int winLen = std::min(bufLen, nSamp - offset);

        // Window slices
        MatrixXd cInWin  = cIn.middleCols(offset, winLen);   // N_in  × winLen
        MatrixXd cOutWin = cOut.middleCols(offset, winLen);  // N_out × winLen

        // ---- Temporal tSSS projection ----
        // SVD of external coefficient matrix (column = time point):
        // cOutWin = U * S * V^T   (N_out × winLen)
        // Right singular vectors V (winLen × min) form the temporal subspace of external signals.
        JacobiSVD<MatrixXd> svd(cOutWin, ComputeThinU | ComputeThinV);
        const VectorXd& sv   = svd.singularValues();
        const MatrixXd& V    = svd.matrixV();   // winLen × rank_out

        // Determine how many external temporal components to suppress
        double svMax = (sv.size() > 0) ? sv(0) : 0.0;
        if (svMax < 1e-30) {
            offset += winLen;
            continue;
        }

        // Build temporal projector to remove the correlated subspace
        // P_remove: (winLen × winLen) = V_r * V_r^T
        // Applied: cInWin_clean = cInWin * (I - P_remove)
        MatrixXd Vr;   // winLen × n_remove
        int nRemove = 0;
        for (int k = 0; k < sv.size(); ++k) {
            if (sv(k) / svMax > dCorrLimit) {
                ++nRemove;
            } else {
                break;
            }
        }

        if (nRemove > 0) {
            Vr = V.leftCols(nRemove);   // winLen × nRemove
            // cInWin_clean = cInWin * (I - Vr * Vr^T)
            MatrixXd cInWinClean = cInWin - cInWin * (Vr * Vr.transpose());
            cIn.middleCols(offset, winLen) = cInWinClean;
        }

        offset += winLen;
    }

    // Reconstruct cleaned MEG data from internal expansion
    MatrixXd megTsss = basis.matSin * cIn;   // n_meg × nSamp

    // Write back
    for (int i = 0; i < nMeg; ++i) {
        matOut.row(basis.megChannelIdx[i]) = megTsss.row(i);
    }

    return matOut;
}
