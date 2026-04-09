//=============================================================================================================
/**
 * @file     kmeans.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh, Gabriel Motta. All rights reserved.
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
 * @brief    Definition of the KMeans class
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "kmeans.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

KMeans::KMeans(QString distance,
               QString start,
               qint32 replicates,
               QString emptyact,
               bool online,
               qint32 maxit)
: m_distance(distanceFromString(distance.toStdString()))
, m_start(startFromString(start.toStdString()))
, m_emptyact(emptyactFromString(emptyact.toStdString()))
, m_iReps(std::max(replicates, qint32(1)))
, m_iMaxit(maxit)
, m_bOnline(online)
, m_rng(std::random_device{}())
, emptyErrCnt(0)
, iter(0)
, k(0)
, n(0)
, p(0)
, totsumD(0)
, prevtotsumD(0)
{
}

//=============================================================================================================

KMeans::KMeans(KMeansDistance distance,
               KMeansStart start,
               qint32 replicates,
               KMeansEmptyAction emptyact,
               bool online,
               qint32 maxit)
: m_distance(distance)
, m_start(start)
, m_emptyact(emptyact)
, m_iReps(std::max(replicates, qint32(1)))
, m_iMaxit(maxit)
, m_bOnline(online)
, m_rng(std::random_device{}())
, emptyErrCnt(0)
, iter(0)
, k(0)
, n(0)
, p(0)
, totsumD(0)
, prevtotsumD(0)
{
}

//=============================================================================================================

KMeansDistance KMeans::distanceFromString(const std::string& name)
{
    if (name == "cityblock")    return KMeansDistance::CityBlock;
    if (name == "cosine")       return KMeansDistance::Cosine;
    if (name == "correlation")  return KMeansDistance::Correlation;
    if (name == "hamming")      return KMeansDistance::Hamming;
    return KMeansDistance::SquaredEuclidean;
}

KMeansStart KMeans::startFromString(const std::string& name)
{
    if (name == "uniform")  return KMeansStart::Uniform;
    if (name == "cluster")  return KMeansStart::Cluster;
    return KMeansStart::Sample;
}

KMeansEmptyAction KMeans::emptyactFromString(const std::string& name)
{
    if (name == "drop")       return KMeansEmptyAction::Drop;
    if (name == "singleton")  return KMeansEmptyAction::Singleton;
    return KMeansEmptyAction::Error;
}

//=============================================================================================================

bool KMeans::calculate(const MatrixXd& X_in,
                       qint32 kClusters,
                       VectorXi& idx,
                       MatrixXd& C,
                       VectorXd& sumD,
                       MatrixXd& D)
{
    if (kClusters < 1)
        return false;

    // Work on a local copy only when normalization is needed
    MatrixXd X = X_in;

    k = kClusters;
    n = X.rows();
    p = X.cols();

    if (m_distance == KMeansDistance::Cosine)
    {
        // Normalize each row to unit length for cosine distance
        VectorXd Xnorm = X.array().pow(2).rowwise().sum().sqrt();
        for (qint32 i = 0; i < n; ++i)
        {
            if (Xnorm(i) > 0)
                X.row(i) /= Xnorm(i);
        }
    }
    else if (m_distance == KMeansDistance::Correlation)
    {
        // Mean-center each row, then normalize to unit length
        X.array() -= (X.rowwise().sum().array() / static_cast<double>(p)).replicate(1, p);
        VectorXd Xnorm = X.array().pow(2).rowwise().sum().sqrt();
        for (qint32 i = 0; i < n; ++i)
        {
            if (Xnorm(i) > 0)
                X.row(i) /= Xnorm(i);
        }
    }

    // Set up uniform initialization bounds if needed
    RowVectorXd Xmins, Xmaxs;
    if (m_start == KMeansStart::Uniform)
    {
        if (m_distance == KMeansDistance::Hamming)
        {
            qWarning("KMeans: Uniform initialization is not supported for Hamming distance.");
            return false;
        }
        Xmins = X.colwise().minCoeff();
        Xmaxs = X.colwise().maxCoeff();
    }

    // Prepare online-update workspace
    if (m_bOnline)
    {
        Del = MatrixXd::Constant(n, k, std::numeric_limits<double>::quiet_NaN());
    }

    double totsumDBest = std::numeric_limits<double>::max();
    emptyErrCnt = 0;

    VectorXi idxBest;
    MatrixXd Cbest;
    VectorXd sumDBest;
    MatrixXd Dbest;

    std::uniform_int_distribution<qint32> sampleDist(0, n - 1);

    for (qint32 rep = 0; rep < m_iReps; ++rep)
    {
        // --- Initialize centroids ---
        if (m_start == KMeansStart::Uniform)
        {
            C = MatrixXd::Zero(k, p);
            for (qint32 i = 0; i < k; ++i)
            {
                for (qint32 j = 0; j < p; ++j)
                {
                    std::uniform_real_distribution<double> dist(Xmins[j], Xmaxs[j]);
                    C(i, j) = dist(m_rng);
                }
            }
            if (m_distance == KMeansDistance::Correlation)
                C.array() -= (C.array().rowwise().sum() / p).replicate(1, p).array();
        }
        else if (m_start == KMeansStart::Sample)
        {
            C = MatrixXd::Zero(k, p);
            for (qint32 i = 0; i < k; ++i)
                C.row(i) = X.row(sampleDist(m_rng));
        }

        // Compute initial distances and assignments
        D = distfun(X, C);
        idx = VectorXi::Zero(n);
        d = VectorXd::Zero(n);

        for (qint32 i = 0; i < n; ++i)
            d[i] = D.row(i).minCoeff(&idx[i]);

        m = VectorXi::Zero(k);
        for (qint32 i = 0; i < n; ++i)
            ++m[idx[i]];

        try
        {
            // Phase 1: batch reassignments
            bool converged = batchUpdate(X, C, idx);

            // Phase 2: single reassignments
            if (m_bOnline)
                converged = onlineUpdate(X, C, idx);

            if (!converged)
                qWarning("KMeans: Failed to converge during replicate %d.", rep);

            // Recompute distances for non-empty clusters only
            VectorXi nonempties = (m.array() > 0).cast<int>();
            qint32 count = nonempties.sum();

            MatrixXd C_tmp(count, C.cols());
            qint32 ci = 0;
            for (qint32 i = 0; i < k; ++i)
                if (nonempties[i])
                    C_tmp.row(ci++) = C.row(i);

            MatrixXd D_tmp = distfun(X, C_tmp);
            ci = 0;
            for (qint32 i = 0; i < k; ++i)
            {
                if (nonempties[i])
                {
                    D.col(i) = D_tmp.col(ci);
                    C.row(i) = C_tmp.row(ci);
                    ++ci;
                }
            }

            // Per-point distance to assigned centroid
            d = VectorXd::Zero(n);
            for (qint32 i = 0; i < n; ++i)
                d[i] = D(i, idx[i]);

            // Cluster-wise sum of distances
            sumD = VectorXd::Zero(k);
            for (qint32 i = 0; i < n; ++i)
                sumD[idx[i]] += d[i];

            totsumD = sumD.sum();

            // Keep the best replicate
            if (totsumD < totsumDBest)
            {
                totsumDBest = totsumD;
                idxBest = idx;
                Cbest = C;
                sumDBest = sumD;
                Dbest = D;
            }
        }
        catch (int)
        {
            if (m_iReps == 1)
                return false;

            ++emptyErrCnt;
            if (emptyErrCnt == m_iReps)
                return false;
        }
    }

    idx = idxBest;
    C = Cbest;
    sumD = sumDBest;
    D = Dbest;

    return true;
}

//=============================================================================================================

bool KMeans::batchUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx)
{
    // Every point moved, every cluster will need an update
    qint32 i = 0;
    VectorXi moved(n);
    for (i = 0; i < n; ++i)
        moved[i] = i;

    VectorXi changed(k);
    for (i = 0; i < k; ++i)
        changed[i] = i;

    previdx = VectorXi::Zero(n);
    prevtotsumD = std::numeric_limits<double>::max();

    MatrixXd D = MatrixXd::Zero(n, k);

    iter = 0;
    bool converged = false;
    while (true)
    {
        ++iter;

        // Recompute centroids for changed clusters and their distances
        MatrixXd C_new;
        VectorXi m_new;
        gcentroids(X, idx, changed, C_new, m_new);
        MatrixXd D_new = distfun(X, C_new);

        for (qint32 i = 0; i < changed.rows(); ++i)
        {
            C.row(changed[i]) = C_new.row(i);
            D.col(changed[i]) = D_new.col(i);
            m[changed[i]] = m_new[i];
        }

        // Handle clusters that just lost all members
        VectorXi empties = VectorXi::Zero(changed.rows());
        for (qint32 i = 0; i < changed.rows(); ++i)
            if (m(i) == 0)
                empties[i] = 1;

        if (empties.sum() > 0)
        {
            if (m_emptyact == KMeansEmptyAction::Error)
            {
                return converged;
            }
            // Drop and Singleton actions: not yet implemented (kept as no-op)
        }

        // Total sum of distances for the current configuration
        totsumD = 0;
        for (qint32 i = 0; i < n; ++i)
            totsumD += D(i, idx[i]);

        // Cycle detection: if objective did not decrease, revert last step
        if (prevtotsumD <= totsumD)
        {
            idx = previdx;
            MatrixXd C_rev;
            VectorXi m_rev;
            gcentroids(X, idx, changed, C_rev, m_rev);
            C.block(0, 0, k, C.cols()) = C_rev;
            m.block(0, 0, k, 1) = m_rev;
            --iter;
            break;
        }

        if (iter >= m_iMaxit)
            break;

        // Reassign points to nearest centroid
        previdx = idx;
        prevtotsumD = totsumD;

        VectorXi nidx(n);
        for (qint32 i = 0; i < n; ++i)
            d[i] = D.row(i).minCoeff(&nidx[i]);

        // Determine which points moved
        std::vector<int> movedVec;
        movedVec.reserve(n);
        for (qint32 i = 0; i < n; ++i)
        {
            if (nidx[i] != previdx[i])
                movedVec.push_back(i);
        }

        // Resolve ties in favor of not moving
        std::vector<int> movedFinal;
        movedFinal.reserve(movedVec.size());
        for (int mi : movedVec)
        {
            if (D(mi, previdx[mi]) > d[mi])
                movedFinal.push_back(mi);
        }

        if (movedFinal.empty())
        {
            converged = true;
            break;
        }

        for (int mi : movedFinal)
            idx[mi] = nidx[mi];

        // Find clusters that gained or lost members
        std::vector<int> tmp;
        tmp.reserve(2 * movedFinal.size());
        for (int mi : movedFinal)
        {
            tmp.push_back(idx[mi]);
            tmp.push_back(previdx[mi]);
        }
        std::sort(tmp.begin(), tmp.end());
        tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());

        changed.resize(tmp.size());
        for (size_t i = 0; i < tmp.size(); ++i)
            changed[i] = tmp[i];
    }
    return converged;
}

//=============================================================================================================

bool KMeans::onlineUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx)
{
    // Initialize city-block median tracking if needed
    MatrixXd Xmid1, Xmid2;
    if (m_distance == KMeansDistance::CityBlock)
    {
        Xmid1 = MatrixXd::Zero(k, p);
        Xmid2 = MatrixXd::Zero(k, p);
        for (qint32 i = 0; i < k; ++i)
        {
            if (m[i] > 0)
            {
                MatrixXd Xsorted(m[i], p);
                qint32 c = 0;
                for (qint32 j = 0; j < n; ++j)
                    if (idx[j] == i)
                        Xsorted.row(c++) = X.row(j);

                for (qint32 j = 0; j < p; ++j)
                    std::sort(Xsorted.col(j).data(), Xsorted.col(j).data() + Xsorted.rows());

                qint32 nn = static_cast<qint32>(std::floor(0.5 * m[i])) - 1;
                if ((m[i] % 2) == 0)
                {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn + 1);
                }
                else if (m[i] > 1)
                {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn + 2);
                }
                else
                {
                    Xmid1.row(i) = Xsorted.row(0);
                    Xmid2.row(i) = Xsorted.row(0);
                }
            }
        }
    }

    // Build list of non-empty clusters
    VectorXi changed(m.rows());
    qint32 count = 0;
    for (qint32 i = 0; i < m.rows(); ++i)
        if (m[i] > 0)
            changed[count++] = i;
    changed.conservativeResize(count);

    qint32 lastmoved = 0;
    qint32 nummoved = 0;
    qint32 iter1 = iter;
    bool converged = false;

    while (iter < m_iMaxit)
    {
        // Compute reassignment criterion Del for changed clusters
        if (m_distance == KMeansDistance::SquaredEuclidean)
        {
            for (qint32 j = 0; j < changed.rows(); ++j)
            {
                qint32 i = changed[j];
                VectorXi mbrs = VectorXi::Zero(n);
                for (qint32 l = 0; l < n; ++l)
                    if (idx[l] == i)
                        mbrs[l] = 1;

                VectorXi sgn = 1 - 2 * mbrs.array();
                if (m[i] == 1)
                    for (qint32 l = 0; l < n; ++l)
                        if (mbrs[l])
                            sgn[l] = 0;

                Del.col(i) = (static_cast<double>(m[i]) / (static_cast<double>(m[i]) + sgn.cast<double>().array()));
                Del.col(i).array() *= (X.rowwise() - C.row(i)).array().pow(2).rowwise().sum().array();
            }
        }
        else if (m_distance == KMeansDistance::CityBlock)
        {
            for (qint32 j = 0; j < changed.rows(); ++j)
            {
                qint32 i = changed[j];
                if (m(i) % 2 == 0)
                {
                    MatrixXd ldist = Xmid1.row(i).replicate(n, 1) - X;
                    MatrixXd rdist = X - Xmid2.row(i).replicate(n, 1);
                    VectorXd mbrs = VectorXd::Zero(n);
                    for (qint32 l = 0; l < n; ++l)
                        if (idx[l] == i)
                            mbrs[l] = 1;
                    MatrixXd sgn = ((-2 * mbrs).array() + 1).replicate(1, p);
                    rdist = sgn.array() * rdist.array();
                    ldist = sgn.array() * ldist.array();

                    for (qint32 l = 0; l < n; ++l)
                    {
                        double sum = 0;
                        for (qint32 h = 0; h < p; ++h)
                            sum += std::max(0.0, std::max(rdist(l, h), ldist(l, h)));
                        Del(l, i) = sum;
                    }
                }
                else
                {
                    Del.col(i) = (X.rowwise() - C.row(i)).array().abs().rowwise().sum();
                }
            }
        }
        else if (m_distance == KMeansDistance::Cosine || m_distance == KMeansDistance::Correlation)
        {
            MatrixXd normC = C.array().pow(2).rowwise().sum().sqrt();
            for (qint32 j = 0; j < changed.rows(); ++j)
            {
                qint32 i = changed[j];
                MatrixXd XCi = X * C.row(i).transpose();

                VectorXi mbrs = VectorXi::Zero(n);
                for (qint32 l = 0; l < n; ++l)
                    if (idx[l] == i)
                        mbrs[l] = 1;

                VectorXi sgn = 1 - 2 * mbrs.array();
                double A = static_cast<double>(m[i]) * normC(i, 0);
                double B = A * A;

                Del.col(i) = 1 + sgn.cast<double>().array() *
                    (A - (B + 2 * sgn.cast<double>().array() * m[i] * XCi.array() + 1).sqrt());
            }
        }
        // Hamming: not yet implemented

        // Find best move for each point
        previdx = idx;
        prevtotsumD = totsumD;

        VectorXi nidx = VectorXi::Zero(n);
        VectorXd minDel = VectorXd::Zero(n);
        for (qint32 i = 0; i < n; ++i)
            minDel[i] = Del.row(i).minCoeff(&nidx[i]);

        // Identify points that would move
        std::vector<int> movedVec;
        movedVec.reserve(n);
        for (qint32 i = 0; i < n; ++i)
            if (previdx[i] != nidx[i])
                movedVec.push_back(i);

        // Resolve ties in favor of not moving
        std::vector<int> movedFinal;
        movedFinal.reserve(movedVec.size());
        for (int mi : movedVec)
            if (Del(mi, previdx[mi]) > minDel(mi))
                movedFinal.push_back(mi);

        if (movedFinal.empty())
        {
            if ((iter == iter1) || nummoved > 0)
                ++iter;
            converged = true;
            break;
        }

        // Pick the next move in cyclic order
        int bestMoved = movedFinal[0];
        int bestDist = ((movedFinal[0] - lastmoved) % n + n) % n;
        for (size_t i = 1; i < movedFinal.size(); ++i)
        {
            int d_i = ((movedFinal[i] - lastmoved) % n + n) % n;
            if (d_i < bestDist)
            {
                bestDist = d_i;
                bestMoved = movedFinal[i];
            }
        }
        int movedPt = bestMoved;

        if (movedPt <= lastmoved)
        {
            ++iter;
            if (iter >= m_iMaxit)
                break;
            nummoved = 0;
        }
        ++nummoved;
        lastmoved = movedPt;

        qint32 oidx = idx[movedPt];
        qint32 nidx_pt = nidx[movedPt];
        totsumD += Del(movedPt, nidx_pt) - Del(movedPt, oidx);

        idx[movedPt] = nidx_pt;
        m(nidx_pt) += 1;
        m(oidx) -= 1;

        // Update centroids for the affected clusters
        if (m_distance == KMeansDistance::SquaredEuclidean)
        {
            C.row(nidx_pt) += (X.row(movedPt) - C.row(nidx_pt)) / m[nidx_pt];
            C.row(oidx)    -= (X.row(movedPt) - C.row(oidx)) / m[oidx];
        }
        else if (m_distance == KMeansDistance::CityBlock)
        {
            VectorXi onidx(2);
            onidx << oidx, nidx_pt;

            for (qint32 h = 0; h < 2; ++h)
            {
                qint32 ci = onidx[h];
                MatrixXd Xsorted(m[ci], p);
                qint32 c = 0;
                for (qint32 j = 0; j < n; ++j)
                    if (idx[j] == ci)
                        Xsorted.row(c++) = X.row(j);

                for (qint32 j = 0; j < p; ++j)
                    std::sort(Xsorted.col(j).data(), Xsorted.col(j).data() + Xsorted.rows());

                qint32 nn = static_cast<qint32>(std::floor(0.5 * m[ci])) - 1;
                if ((m[ci] % 2) == 0)
                {
                    C.row(ci) = 0.5 * (Xsorted.row(nn) + Xsorted.row(nn + 1));
                    Xmid1.row(ci) = Xsorted.row(nn);
                    Xmid2.row(ci) = Xsorted.row(nn + 1);
                }
                else
                {
                    C.row(ci) = Xsorted.row(nn + 1);
                    if (m(ci) > 1)
                    {
                        Xmid1.row(ci) = Xsorted.row(nn);
                        Xmid2.row(ci) = Xsorted.row(nn + 2);
                    }
                    else
                    {
                        Xmid1.row(ci) = Xsorted.row(0);
                        Xmid2.row(ci) = Xsorted.row(0);
                    }
                }
            }
        }
        else if (m_distance == KMeansDistance::Cosine || m_distance == KMeansDistance::Correlation)
        {
            C.row(nidx_pt).array() += (X.row(movedPt) - C.row(nidx_pt)).array() / m[nidx_pt];
            C.row(oidx).array()    += (X.row(movedPt) - C.row(oidx)).array() / m[oidx];
        }

        VectorXi sorted_onidx(2);
        sorted_onidx << oidx, nidx_pt;
        std::sort(sorted_onidx.data(), sorted_onidx.data() + sorted_onidx.rows());
        changed = sorted_onidx;
    }

    return converged;
}

//=============================================================================================================

MatrixXd KMeans::distfun(const MatrixXd& X, const MatrixXd& C)
{
    const qint32 nclusts = C.rows();
    MatrixXd D = MatrixXd::Zero(n, nclusts);

    switch (m_distance)
    {
    case KMeansDistance::SquaredEuclidean:
        for (qint32 i = 0; i < nclusts; ++i)
            D.col(i) = (X.rowwise() - C.row(i)).rowwise().squaredNorm();
        break;

    case KMeansDistance::CityBlock:
        for (qint32 i = 0; i < nclusts; ++i)
            D.col(i) = (X.rowwise() - C.row(i)).cwiseAbs().rowwise().sum();
        break;

    case KMeansDistance::Cosine:
    case KMeansDistance::Correlation:
    {
        VectorXd normC = C.rowwise().norm();
        for (qint32 i = 0; i < nclusts; ++i)
        {
            RowVectorXd C_normed = C.row(i) / normC(i);
            D.col(i) = (1.0 - (X * C_normed.transpose()).array()).cwiseMax(0.0);
        }
        break;
    }

    case KMeansDistance::Hamming:
        for (qint32 i = 0; i < nclusts; ++i)
            D.col(i) = (X.rowwise() - C.row(i)).cwiseAbs().rowwise().sum() / p;
        break;
    }

    return D;
}

//=============================================================================================================

void KMeans::gcentroids(const MatrixXd& X, const VectorXi& index, const VectorXi& clusts,
                        MatrixXd& centroids, VectorXi& counts)
{
    const qint32 num = clusts.rows();
    centroids = MatrixXd::Constant(num, p, std::numeric_limits<double>::quiet_NaN());
    counts = VectorXi::Zero(num);

    for (qint32 i = 0; i < num; ++i)
    {
        // Collect member indices for cluster clusts[i]
        std::vector<int> members;
        members.reserve(n);
        for (qint32 j = 0; j < index.rows(); ++j)
            if (index[j] == clusts[i])
                members.push_back(j);

        counts[i] = static_cast<qint32>(members.size());
        if (members.empty())
            continue;

        switch (m_distance)
        {
        case KMeansDistance::SquaredEuclidean:
        case KMeansDistance::Cosine:
        case KMeansDistance::Correlation:
        {
            centroids.row(i) = RowVectorXd::Zero(p);
            for (int j : members)
                centroids.row(i) += X.row(j);
            centroids.row(i) /= counts[i];
            break;
        }

        case KMeansDistance::CityBlock:
        {
            MatrixXd Xsorted(counts[i], p);
            qint32 c = 0;
            for (int j : members)
                Xsorted.row(c++) = X.row(j);

            for (qint32 j = 0; j < p; ++j)
                std::sort(Xsorted.col(j).data(), Xsorted.col(j).data() + Xsorted.rows());

            qint32 nn = static_cast<qint32>(std::floor(0.5 * counts[i])) - 1;
            if (counts[i] % 2 == 0)
                centroids.row(i) = 0.5 * (Xsorted.row(nn) + Xsorted.row(nn + 1));
            else
                centroids.row(i) = Xsorted.row(nn + 1);
            break;
        }

        case KMeansDistance::Hamming:
            // Not yet implemented
            break;
        }
    }
}
