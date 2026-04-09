//=============================================================================================================
/**
 * @file     inv_minimum_norm.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     February, 2013
 *
 * @section  LICENSE
 *
 * Copyright (C) 2013, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    Definition of the InvMinimumNorm Class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "inv_minimum_norm.h"

#include <inv/inv_source_estimate.h>
#include <fiff/fiff_evoked.h>
#include <math/linalg.h>

#include <iostream>
#include <cmath>
#include <algorithm>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <Eigen/SVD>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace INVLIB;
using namespace UTILSLIB;
using namespace FIFFLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

InvMinimumNorm::InvMinimumNorm(const MNEInverseOperator &p_inverseOperator, float lambda, const QString method)
: m_inverseOperator(p_inverseOperator)
, m_beLoreta(false)
, m_iELoretaMaxIter(20)
, m_dELoretaEps(1e-6)
, m_bELoretaForceEqual(false)
, inverseSetup(false)
{
    this->setRegularization(lambda);
    this->setMethod(method);
}

//=============================================================================================================

InvMinimumNorm::InvMinimumNorm(const MNEInverseOperator &p_inverseOperator, float lambda, bool dSPM, bool sLORETA)
: m_inverseOperator(p_inverseOperator)
, m_beLoreta(false)
, m_iELoretaMaxIter(20)
, m_dELoretaEps(1e-6)
, m_bELoretaForceEqual(false)
, inverseSetup(false)
{
    this->setRegularization(lambda);
    this->setMethod(dSPM, sLORETA);
}

//=============================================================================================================

InvSourceEstimate InvMinimumNorm::calculateInverse(const FiffEvoked &p_fiffEvoked, bool pick_normal)
{
    //
    //   Set up the inverse according to the parameters
    //
    qint32 nave = p_fiffEvoked.nave;

    if(!m_inverseOperator.check_ch_names(p_fiffEvoked.info)) {
        qWarning("Channel name check failed.");
        return InvSourceEstimate();
    }

    doInverseSetup(nave,pick_normal);

    //
    //   Pick the correct channels from the data
    //
    FiffEvoked t_fiffEvoked = p_fiffEvoked.pick_channels(inv.noise_cov->names);

    qInfo("Picked %d channels from the data", t_fiffEvoked.info.nchan);

    //Results
    float tmin = p_fiffEvoked.times[0];
    float tstep = 1/t_fiffEvoked.info.sfreq;

    return calculateInverse(t_fiffEvoked.data, tmin, tstep, pick_normal);
}

//=============================================================================================================

InvSourceEstimate InvMinimumNorm::calculateInverse(const MatrixXd &data, float tmin, float tstep, bool pick_normal) const
{
    if(!inverseSetup)
    {
        qWarning("InvMinimumNorm::calculateInverse - Inverse not setup -> call doInverseSetup first!");
        return InvSourceEstimate();
    }

    if(K.cols() != data.rows()) {
        qWarning() << "InvMinimumNorm::calculateInverse - Dimension mismatch between K.cols() and data.rows() -" << K.cols() << "and" << data.rows();
        return InvSourceEstimate();
    }

    MatrixXd sol = K * data; //apply imaging kernel

    if (inv.source_ori == FIFFV_MNE_FREE_ORI && pick_normal == false)
    {
        qInfo("combining the current components...");

        MatrixXd sol1(sol.rows()/3,sol.cols());
        for(qint32 i = 0; i < sol.cols(); ++i)
        {
            VectorXd tmp = Linalg::combine_xyz(sol.col(i));
            sol1.block(0,i,sol.rows()/3,1) = tmp.cwiseSqrt();
        }
        sol.resize(sol1.rows(),sol1.cols());
        sol = sol1;
    }

    if (m_bdSPM)
    {
        qInfo("(dSPM)...");
        sol = inv.noisenorm*sol;
    }
    else if (m_bsLORETA)
    {
        qInfo("(sLORETA)...");
        sol = inv.noisenorm*sol;
    }
    else if (m_beLoreta)
    {
        qInfo("(eLORETA)...");
        sol = inv.noisenorm*sol;
    }
    qInfo("[done]");

    //Results
    VectorXi p_vecVertices(inv.src[0].vertno.size() + inv.src[1].vertno.size());
    p_vecVertices << inv.src[0].vertno, inv.src[1].vertno;

//    VectorXi p_vecVertices();
//    for(qint32 h = 0; h < inv.src.size(); ++h)
//        t_qListVertices.push_back(inv.src[h].vertno);

    return InvSourceEstimate(sol, p_vecVertices, tmin, tstep);
}

//=============================================================================================================

void InvMinimumNorm::doInverseSetup(qint32 nave, bool pick_normal)
{
    //
    //   Set up the inverse according to the parameters
    //
    inv = m_inverseOperator.prepare_inverse_operator(nave, m_fLambda, m_bdSPM || m_beLoreta, m_bsLORETA);

    // For eLORETA: recompute source covariance weights before assembling kernel
    if(m_beLoreta) {
        computeELoreta();
    }

    qInfo("Computing inverse...");
    inv.assemble_kernel(label, m_sMethod, pick_normal, K, noise_norm, vertno);

    std::cout << "K " << K.rows() << " x " << K.cols() << std::endl;

    inverseSetup = true;
}

//=============================================================================================================

const char* InvMinimumNorm::getName() const
{
    return "Minimum Norm Estimate";
}

//=============================================================================================================

const MNESourceSpaces& InvMinimumNorm::getSourceSpace() const
{
    return m_inverseOperator.src;
}

//=============================================================================================================

void InvMinimumNorm::setMethod(QString method)
{
    if(method.compare("MNE") == 0)
        setMethod(false, false);
    else if(method.compare("dSPM") == 0)
        setMethod(true, false);
    else if(method.compare("sLORETA") == 0)
        setMethod(false, true);
    else if(method.compare("eLORETA") == 0)
        setMethod(false, false, true);
    else
    {
        qWarning("Method not recognized!");
        method = "dSPM";
        setMethod(true, false);
    }

    qInfo("\tSet minimum norm method to %s.", method.toUtf8().constData());
}

//=============================================================================================================

void InvMinimumNorm::setMethod(bool dSPM, bool sLORETA, bool eLoreta)
{
    int nActive = (dSPM ? 1 : 0) + (sLORETA ? 1 : 0) + (eLoreta ? 1 : 0);
    if(nActive > 1)
    {
        qWarning("Only one method can be active at a time! - Activating dSPM");
        dSPM = true;
        sLORETA = false;
        eLoreta = false;
    }

    m_bdSPM = dSPM;
    m_bsLORETA = sLORETA;
    m_beLoreta = eLoreta;

    if(dSPM)
        m_sMethod = QString("dSPM");
    else if(sLORETA)
        m_sMethod = QString("sLORETA");
    else if(eLoreta)
        m_sMethod = QString("eLORETA");
    else
        m_sMethod = QString("MNE");
}

//=============================================================================================================

void InvMinimumNorm::setRegularization(float lambda)
{
    m_fLambda = lambda;
}

//=============================================================================================================

void InvMinimumNorm::setELoretaOptions(int maxIter, double eps, bool forceEqual)
{
    m_iELoretaMaxIter = maxIter;
    m_dELoretaEps = eps;
    m_bELoretaForceEqual = forceEqual;
}

//=============================================================================================================

void InvMinimumNorm::computeELoreta()
{
    //
    // eLORETA: Iteratively compute optimized source covariance weights R
    // so that lambda2 acts consistently across depth.
    //
    // Reference: Pascual-Marqui (2007), Discrete, 3D distributed, linear imaging
    //            methods of electric neuronal activity.
    // Adapted from MNE-Python: mne.minimum_norm._eloreta._compute_eloreta()
    //

    qInfo("Computing eLORETA source weights...");

    // Reassemble the whitened gain matrix G from the SVD of the inverse operator:
    //   G = eigen_fields * diag(sing) * eigen_leads^T
    // where eigen_fields is (n_channels, n_comp) and eigen_leads is (n_sources*n_orient, n_comp),
    // giving G of shape (n_channels, n_sources*n_orient).
    if(!inv.eigen_fields || !inv.eigen_leads) {
        qWarning("InvMinimumNorm::computeELoreta - Inverse operator missing eigen structures!");
        return;
    }

    const MatrixXd &eigenFields = inv.eigen_fields->data;    // (n_channels, n_comp)
    const MatrixXd &eigenLeads = inv.eigen_leads->data;      // (n_sources*n_orient, n_comp)
    const VectorXd &sing = inv.sing;

    // G = eigenFields * diag(sing) * eigenLeads^T
    // (n_channels, n_comp) * (n_comp, n_comp) * (n_comp, n_sources*n_orient) = (n_channels, n_sources*n_orient)
    MatrixXd G = eigenFields * sing.asDiagonal() * eigenLeads.transpose();

    const int nChan = static_cast<int>(G.rows());
    const int nSrc = inv.nsource;
    const int nOrient = static_cast<int>(G.cols()) / nSrc;

    if(nOrient != 1 && nOrient != 3) {
        qWarning("InvMinimumNorm::computeELoreta - Unexpected n_orient: %d", nOrient);
        return;
    }

    // Divide by sqrt(source_cov) to undo source covariance weighting
    if(inv.source_cov && inv.source_cov->data.size() > 0) {
        for(int i = 0; i < G.cols(); ++i) {
            double sc = inv.source_cov->data(i, 0);
            if(sc > 0) G.col(i) /= std::sqrt(sc);
        }
    }

    // Restore orientation prior
    VectorXd sourceStd = VectorXd::Ones(G.cols());
    if(inv.orient_prior && inv.orient_prior->data.size() > 0) {
        for(int i = 0; i < G.cols() && i < inv.orient_prior->data.rows(); ++i) {
            double op = inv.orient_prior->data(i, 0);
            if(op > 0) sourceStd(i) *= std::sqrt(op);
        }
    }
    for(int i = 0; i < G.cols(); ++i) {
        G.col(i) *= sourceStd(i);
    }

    // Compute rank (number of non-zero singular values)
    int nNonZero = 0;
    for(int i = 0; i < sing.size(); ++i) {
        if(std::abs(sing(i)) > 1e-10 * sing(0))
            ++nNonZero;
    }

    double lambda2 = static_cast<double>(m_fLambda);

    // Initialize weight matrix R
    // For fixed orientation (or force_equal): R is a diagonal vector (nSrc * nOrient)
    // For free orientation: R is block-diagonal (nSrc x 3 x 3)
    const bool useScalar = (nOrient == 1 || m_bELoretaForceEqual);

    VectorXd R_vec;      // For scalar mode: (nSrc * nOrient)
    std::vector<Matrix3d> R_mat;  // For matrix mode: nSrc x (3x3)

    if(useScalar) {
        R_vec = VectorXd::Ones(nSrc * nOrient);
        // Apply prior: R *= sourceStd^2
        for(int i = 0; i < R_vec.size(); ++i) {
            R_vec(i) *= sourceStd(i) * sourceStd(i);
        }
    } else {
        R_mat.resize(nSrc);
        for(int s = 0; s < nSrc; ++s) {
            R_mat[s] = Matrix3d::Identity();
            // Apply prior as outer product
            for(int a = 0; a < 3; ++a) {
                for(int b = 0; b < 3; ++b) {
                    R_mat[s](a, b) *= sourceStd(s * 3 + a) * sourceStd(s * 3 + b);
                }
            }
        }
    }

    qInfo("    Fitting up to %d iterations (n_orient=%d, force_equal=%s)...",
          m_iELoretaMaxIter, nOrient, m_bELoretaForceEqual ? "true" : "false");

    // Lambda for computing G * R * G^T and normalizing
    auto computeGRGt = [&]() -> MatrixXd {
        MatrixXd GRGt;
        if(useScalar) {
            // G_R = G * diag(R)
            MatrixXd GR = G;
            for(int i = 0; i < G.cols(); ++i) {
                GR.col(i) *= R_vec(i);
            }
            GRGt = GR * G.transpose();
        } else {
            // Block multiplication
            MatrixXd RGt = MatrixXd::Zero(nSrc * 3, nChan);
            for(int s = 0; s < nSrc; ++s) {
                // G_s: (nChan, 3)
                MatrixXd Gs = G.middleCols(s * 3, 3);
                // R_s * G_s^T: (3, nChan)
                RGt.middleRows(s * 3, 3) = R_mat[s] * Gs.transpose();
            }
            GRGt = G * RGt;
        }
        // Normalize so trace(GRGt) / nNonZero = 1
        double trace = GRGt.trace();
        double norm = trace / static_cast<double>(nNonZero);
        if(norm > 1e-30) {
            GRGt /= norm;
            if(useScalar) R_vec /= norm;
            else for(auto &Rm : R_mat) Rm /= norm;
        }
        return GRGt;
    };

    MatrixXd GRGt = computeGRGt();

    for(int kk = 0; kk < m_iELoretaMaxIter; ++kk) {
        // 1. Compute inverse of GRGt (stabilized eigendecomposition)
        SelfAdjointEigenSolver<MatrixXd> eig(GRGt);
        VectorXd s = eig.eigenvalues().cwiseAbs();
        MatrixXd u = eig.eigenvectors();

        // Keep top nNonZero eigenvalues
        // Sort descending
        std::vector<int> idx(s.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(), [&s](int a, int b) { return s(a) > s(b); });

        MatrixXd uKeep(nChan, nNonZero);
        VectorXd sKeep(nNonZero);
        for(int i = 0; i < nNonZero && i < static_cast<int>(idx.size()); ++i) {
            uKeep.col(i) = u.col(idx[i]);
            sKeep(i) = s(idx[i]);
        }

        // N = u * diag(1/(s + lambda2)) * u^T
        VectorXd sInv(nNonZero);
        for(int i = 0; i < nNonZero; ++i) {
            sInv(i) = (sKeep(i) > 0) ? 1.0 / (sKeep(i) + lambda2) : 0.0;
        }
        MatrixXd N = uKeep * sInv.asDiagonal() * uKeep.transpose();

        // Save old R for convergence check
        VectorXd R_old_vec;
        std::vector<Matrix3d> R_old_mat;
        if(useScalar) R_old_vec = R_vec;
        else R_old_mat = R_mat;

        // 2. Update R
        if(nOrient == 1) {
            // R_i = 1 / sqrt(sum_j(N * G)_ij * G_ij)
            MatrixXd NG = N * G;   // (nChan, nDipoles)
            for(int i = 0; i < nSrc; ++i) {
                double val = (NG.col(i).array() * G.col(i).array()).sum();
                R_vec(i) = (val > 1e-30) ? 1.0 / std::sqrt(val) : 1.0;
            }
        } else if(m_bELoretaForceEqual) {
            // For force_equal: compute M_s = G_s^T N G_s, then average eigenvalues of M_s^{-1/2}
            for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
                MatrixXd Gs = G.middleCols(s_idx * 3, 3);  // (nChan, 3)
                Matrix3d M = Gs.transpose() * N * Gs;

                SelfAdjointEigenSolver<Matrix3d> eigM(M);
                Vector3d mEig = eigM.eigenvalues();
                double meanInvSqrt = 0;
                for(int d = 0; d < 3; ++d) {
                    meanInvSqrt += (mEig(d) > 1e-30) ? 1.0 / std::sqrt(mEig(d)) : 0.0;
                }
                meanInvSqrt /= 3.0;
                for(int d = 0; d < 3; ++d) {
                    R_vec(s_idx * 3 + d) = meanInvSqrt;
                }
            }
        } else {
            // Free orientation, independent: R_s = sqrtm(G_s^T N G_s)^{-1/2}
            for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
                MatrixXd Gs = G.middleCols(s_idx * 3, 3);
                Matrix3d M = Gs.transpose() * N * Gs;

                // Symmetric matrix power: M^{-1/2}
                SelfAdjointEigenSolver<Matrix3d> eigM(M);
                Vector3d mEig = eigM.eigenvalues();
                Matrix3d mVec = eigM.eigenvectors();
                Vector3d mPow;
                for(int d = 0; d < 3; ++d) {
                    mPow(d) = (mEig(d) > 1e-30) ? std::pow(mEig(d), -0.5) : 0.0;
                }
                R_mat[s_idx] = mVec * mPow.asDiagonal() * mVec.transpose();
            }
        }

        // Reapply prior
        if(useScalar) {
            for(int i = 0; i < R_vec.size(); ++i) {
                R_vec(i) *= sourceStd(i) * sourceStd(i);
            }
        } else {
            for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
                for(int a = 0; a < 3; ++a) {
                    for(int b = 0; b < 3; ++b) {
                        R_mat[s_idx](a, b) *= sourceStd(s_idx * 3 + a) * sourceStd(s_idx * 3 + b);
                    }
                }
            }
        }

        GRGt = computeGRGt();

        // 3. Check convergence
        double deltaNum = 0.0, deltaDen = 0.0;
        if(useScalar) {
            deltaNum = (R_vec - R_old_vec).norm();
            deltaDen = R_old_vec.norm();
        } else {
            for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
                Matrix3d diff = R_mat[s_idx] - R_old_mat[s_idx];
                deltaNum += diff.squaredNorm();
                deltaDen += R_old_mat[s_idx].squaredNorm();
            }
            deltaNum = std::sqrt(deltaNum);
            deltaDen = std::sqrt(deltaDen);
        }
        double delta = (deltaDen > 1e-30) ? deltaNum / deltaDen : 0.0;

        if(delta < m_dELoretaEps) {
            qInfo("    eLORETA converged on iteration %d (delta=%.2e < eps=%.2e)", kk + 1, delta, m_dELoretaEps);
            break;
        }
        if(kk == m_iELoretaMaxIter - 1) {
            qWarning("    eLORETA weight fitting did not converge after %d iterations (delta=%.2e)", m_iELoretaMaxIter, delta);
        }
    }

    // Undo source_std weighting on G
    for(int i = 0; i < G.cols(); ++i) {
        G.col(i) /= sourceStd(i);
    }

    // Compute R^{1/2}
    VectorXd R_sqrt_vec;
    std::vector<Matrix3d> R_sqrt_mat;
    if(useScalar) {
        R_sqrt_vec = R_vec.cwiseSqrt();
    } else {
        R_sqrt_mat.resize(nSrc);
        for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
            SelfAdjointEigenSolver<Matrix3d> eigR(R_mat[s_idx]);
            Vector3d rEig = eigR.eigenvalues();
            Matrix3d rVec = eigR.eigenvectors();
            Vector3d rSqrt;
            for(int d = 0; d < 3; ++d) {
                rSqrt(d) = (rEig(d) > 1e-30) ? std::sqrt(rEig(d)) : 0.0;
            }
            R_sqrt_mat[s_idx] = rVec * rSqrt.asDiagonal() * rVec.transpose();
        }
    }

    // Compute weighted gain: A = G * R^{1/2}
    MatrixXd A = G;
    if(useScalar) {
        for(int i = 0; i < A.cols(); ++i) {
            A.col(i) *= R_sqrt_vec(i);
        }
    } else {
        for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
            MatrixXd Gs = G.middleCols(s_idx * 3, 3);
            A.middleCols(s_idx * 3, 3) = Gs * R_sqrt_mat[s_idx];
        }
    }

    // SVD of A = G * R^{1/2}, shape (nChan, nSrc*nOrient)
    // A = U * Σ * V^T  →  U: (nChan, ncomp), V: (nSrc*nOrient, ncomp)
    JacobiSVD<MatrixXd> svd(A, ComputeThinU | ComputeThinV);
    const VectorXd newSing = svd.singularValues();    // (ncomp,)
    const MatrixXd newU    = svd.matrixU();           // (nChan, ncomp) — new eigen_fields
    const MatrixXd newV    = svd.matrixV();           // (nSrc*nOrient, ncomp)

    // Build R^{1/2}-weighted eigen_leads: weightedLeads[i,:] = R_sqrt[i] * V[i,:]
    MatrixXd weightedLeads = newV;  // (nSrc*nOrient, ncomp)
    if(useScalar) {
        for(int i = 0; i < weightedLeads.rows(); ++i) {
            weightedLeads.row(i) *= R_sqrt_vec(i);
        }
    } else {
        // For each source s: rows [s*3, s*3+3) = R_sqrt_mat[s] * V[s*3, s*3+3)
        for(int s_idx = 0; s_idx < nSrc; ++s_idx) {
            MatrixXd Vs = newV.middleRows(s_idx * 3, 3);  // (3, ncomp)
            weightedLeads.middleRows(s_idx * 3, 3) = R_sqrt_mat[s_idx] * Vs;
        }
    }

    // Update inverse operator:
    //   eigen_fields: (nChan, ncomp)          = U
    //   eigen_leads:  (nSrc*nOrient, ncomp)   = R_sqrt * V  (eLORETA-weighted)
    //   eigen_leads_weighted = true  →  prepare_inverse_operator uses K = eigenLeads * trans directly
    inv.sing                  = newSing;
    inv.eigen_fields->data    = newU;
    inv.eigen_leads->data     = weightedLeads;
    inv.eigen_leads_weighted  = true;

    // Recompute reginv: σ / (σ² + λ²)
    VectorXd reginv(newSing.size());
    for(int i = 0; i < newSing.size(); ++i) {
        const double s2 = newSing(i) * newSing(i);
        reginv(i) = (s2 > 1e-30) ? newSing(i) / (s2 + lambda2) : 0.0;
    }
    inv.reginv = reginv;

    qInfo("    eLORETA inverse operator updated. [done]");
}
