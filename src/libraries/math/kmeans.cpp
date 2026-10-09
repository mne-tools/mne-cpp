//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     kmeans.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    K-means batch/online update kernels, replicate driver and empty-cluster handling.
 *
 * This translation unit implements the inner loop of @ref UTILSLIB::KMeans:
 * the batch Lloyd phase (compute pairwise distances, reassign points,
 * recompute centroids), the optional online single-point refinement
 * phase used to break out of shallow local minima, and the
 * empty-cluster dispatch (error / drop / singleton-from-farthest-point)
 * driven by the @ref KMeansEmptyAction enum. Distance evaluation is
 * factored per metric (squared Euclidean, city-block, cosine,
 * correlation, Hamming) so that each replicate pays only for the
 * metric the caller actually selected.
 *
 * The driver runs @c m_iReps independent replicates seeded according to
 * @ref KMeansStart, keeps the one with the smallest total
 * within-cluster distortion and returns its labels and centroids,
 * matching the convention used by MATLAB's @c kmeans.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "kmeans.h"

#include <cmath>
#include <iostream>
#include <algorithm>
#include <numeric>
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

namespace
{

/** Smallest non-NaN entry and its column, like MATLAB's min (dropped clusters are NaN). */
double nanMin(const Ref<const RowVectorXd>& row, int& col)
{
    col = -1;
    double best = std::numeric_limits<double>::quiet_NaN();
    for (Index j = 0; j < row.size(); ++j) {
        if (!std::isnan(row[j]) && (col < 0 || row[j] < best)) {
            best = row[j];
            col = static_cast<int>(j);
        }
    }
    return best;
}

} // namespace

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
    if (name == "cityblock")
        return KMeansDistance::CityBlock;
    if (name == "cosine")
        return KMeansDistance::Cosine;
    if (name == "correlation")
        return KMeansDistance::Correlation;
    if (name == "hamming")
        return KMeansDistance::Hamming;
    return KMeansDistance::SquaredEuclidean;
}

KMeansStart KMeans::startFromString(const std::string& name)
{
    if (name == "uniform")
        return KMeansStart::Uniform;
    if (name == "cluster")
        return KMeansStart::Cluster;
    return KMeansStart::Sample;
}

KMeansEmptyAction KMeans::emptyactFromString(const std::string& name)
{
    if (name == "drop")
        return KMeansEmptyAction::Drop;
    if (name == "singleton")
        return KMeansEmptyAction::Singleton;
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
    if (kClusters < 1 || X_in.rows() < kClusters)
        return false;

    const MatrixXd X = normalizedRows(X_in);
    k = kClusters;
    n = X.rows();
    p = X.cols();

    // Set up uniform initialization bounds if needed
    RowVectorXd Xmins, Xmaxs;
    if (m_start == KMeansStart::Uniform) {
        if (m_distance == KMeansDistance::Hamming) {
            qWarning("KMeans: Uniform initialization is not supported for Hamming distance.");
            return false;
        }
        Xmins = X.colwise().minCoeff();
        Xmaxs = X.colwise().maxCoeff();
    }

    double totsumDBest = std::numeric_limits<double>::max();
    emptyErrCnt = 0;

    VectorXi idxBest;
    MatrixXd Cbest;
    VectorXd sumDBest;
    MatrixXd Dbest;

    std::uniform_int_distribution<qint32> sampleDist(0, n - 1);

    for (qint32 rep = 0; rep < m_iReps; ++rep) {
        // --- Initialize centroids ---
        C = MatrixXd::Zero(k, p);
        if (m_start == KMeansStart::Uniform) {
            for (qint32 i = 0; i < k; ++i) {
                for (qint32 j = 0; j < p; ++j) {
                    std::uniform_real_distribution<double> dist(Xmins[j], Xmaxs[j]);
                    C(i, j) = dist(m_rng);
                }
            }
            if (m_distance == KMeansDistance::Correlation)
                C.array() -= (C.array().rowwise().sum() / p).replicate(1, p).array();
        } else if (m_start == KMeansStart::Sample) {
            for (qint32 i = 0; i < k; ++i)
                C.row(i) = X.row(sampleDist(m_rng));
        } else {
            // MATLAB 'cluster': seed from a 'sample'-initialized clustering of a random 10 % subsample
            std::vector<qint32> order(n);
            std::iota(order.begin(), order.end(), 0);
            std::shuffle(order.begin(), order.end(), m_rng);
            const qint32 nSub = std::max(k, static_cast<qint32>(std::floor(0.1 * n)));
            MatrixXd Xsub(nSub, p);
            for (qint32 i = 0; i < nSub; ++i)
                Xsub.row(i) = X.row(order[i]);
            KMeans preliminary(m_distance, KMeansStart::Sample, 1, m_emptyact, m_bOnline, m_iMaxit);
            VectorXi idxSub;
            VectorXd sumDSub;
            MatrixXd DSub;
            if (!preliminary.calculate(Xsub, k, idxSub, C, sumDSub, DSub)) {
                ++emptyErrCnt;
                if (emptyErrCnt == m_iReps)
                    return false;
                continue;
            }
            // Dropped preliminary clusters restart from a sample point
            for (qint32 i = 0; i < k; ++i)
                if (!C.row(i).allFinite())
                    C.row(i) = X.row(sampleDist(m_rng));
        }

        if (!runReplicate(X, C, idx, sumD, D, rep)) {
            ++emptyErrCnt;
            if (emptyErrCnt == m_iReps)
                return false;
            continue;
        }

        // Keep the best replicate
        if (totsumD < totsumDBest) {
            totsumDBest = totsumD;
            idxBest = idx;
            Cbest = C;
            sumDBest = sumD;
            Dbest = D;
        }
    }

    idx = idxBest;
    C = Cbest;
    sumD = sumDBest;
    D = Dbest;

    return true;
}

//=============================================================================================================

bool KMeans::calculate(const MatrixXd& X_in,
                       const MatrixXd& start,
                       VectorXi& idx,
                       MatrixXd& C,
                       VectorXd& sumD,
                       MatrixXd& D)
{
    if (start.rows() < 1 || start.cols() != X_in.cols() || X_in.rows() < start.rows())
        return false;

    const MatrixXd X = normalizedRows(X_in);
    k = static_cast<qint32>(start.rows());
    n = X.rows();
    p = X.cols();
    emptyErrCnt = 0;
    C = normalizedRows(start);
    return runReplicate(X, C, idx, sumD, D, 0);
}

//=============================================================================================================

MatrixXd KMeans::normalizedRows(const MatrixXd& X) const
{
    if (m_distance != KMeansDistance::Cosine && m_distance != KMeansDistance::Correlation)
        return X;

    MatrixXd Xn = X;
    if (m_distance == KMeansDistance::Correlation)
        Xn.colwise() -= Xn.rowwise().mean();
    const VectorXd norms = Xn.rowwise().norm();
    for (Index i = 0; i < Xn.rows(); ++i) {
        if (norms(i) > 0)
            Xn.row(i) /= norms(i);
    }
    return Xn;
}

//=============================================================================================================

bool KMeans::runReplicate(const MatrixXd& X, MatrixXd& C, VectorXi& idx, VectorXd& sumD, MatrixXd& D, qint32 rep)
{
    if (m_bOnline)
        Del = MatrixXd::Constant(n, k, std::numeric_limits<double>::quiet_NaN());

    // Compute initial distances and assignments
    D = distfun(X, C);
    idx = VectorXi::Zero(n);
    d = VectorXd::Zero(n);

    for (qint32 i = 0; i < n; ++i)
        d[i] = nanMin(D.row(i), idx[i]);

    m = VectorXi::Zero(k);
    for (qint32 i = 0; i < n; ++i)
        ++m[idx[i]];

    // Phase 1: batch reassignments
    bool converged = false;
    if (!batchUpdate(X, C, idx, converged))
        return false;

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
    for (qint32 i = 0; i < k; ++i) {
        if (nonempties[i]) {
            D.col(i) = D_tmp.col(ci);
            C.row(i) = C_tmp.row(ci);
            ++ci;
        } else {
            D.col(i).setConstant(std::numeric_limits<double>::quiet_NaN());
            C.row(i).setConstant(std::numeric_limits<double>::quiet_NaN());
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
    return true;
}

//=============================================================================================================

bool KMeans::batchUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx, bool& converged)
{
    // Every point moved, every cluster will need an update.
    // Both indices are loop-local: a function-scope `i` was shadowed by every
    // later loop in this function.
    VectorXi moved(n);
    for (qint32 i = 0; i < n; ++i)
        moved[i] = i;

    VectorXi changed(k);
    for (qint32 i = 0; i < k; ++i)
        changed[i] = i;

    previdx = VectorXi::Zero(n);
    prevtotsumD = std::numeric_limits<double>::max();

    MatrixXd D = MatrixXd::Zero(n, k);

    iter = 0;
    converged = false;
    while (true) {
        ++iter;

        // Recompute centroids for changed clusters and their distances
        MatrixXd C_new;
        VectorXi m_new;
        gcentroids(X, idx, changed, C_new, m_new);
        MatrixXd D_new = distfun(X, C_new);

        for (qint32 i = 0; i < changed.rows(); ++i) {
            C.row(changed[i]) = C_new.row(i);
            D.col(changed[i]) = D_new.col(i);
            m[changed[i]] = m_new[i];
        }

        // Handle clusters that just lost all members (MATLAB kmeans emptyact)
        std::vector<int> empties;
        for (qint32 i = 0; i < changed.rows(); ++i)
            if (m[changed[i]] == 0)
                empties.push_back(changed[i]);

        if (!empties.empty()) {
            if (m_emptyact == KMeansEmptyAction::Error) {
                qWarning("KMeans: Empty cluster created at iteration %d.", iter);
                return false;
            }
            std::vector<int> changedList(changed.data(), changed.data() + changed.size());
            if (m_emptyact == KMeansEmptyAction::Drop) {
                for (int e : empties)
                    D.col(e).setConstant(std::numeric_limits<double>::quiet_NaN());
                changedList.erase(std::remove_if(changedList.begin(), changedList.end(), [this](int c) { return m[c] == 0; }),
                                  changedList.end());
            } else {
                for (int e : empties) {
                    // The point farthest from its centroid becomes the new singleton cluster
                    qint32 lonely = 0;
                    for (qint32 i = 1; i < n; ++i)
                        if (D(i, idx[i]) > D(lonely, idx[lonely]))
                            lonely = i;
                    qint32 from = idx[lonely];
                    if (m[from] < 2) {
                        for (from = 0; m[from] < 2; ++from) {
                        }
                        for (lonely = 0; idx[lonely] != from; ++lonely) {
                        }
                    }
                    C.row(e) = X.row(lonely);
                    m[e] = 1;
                    idx[lonely] = e;
                    D.col(e) = distfun(X, C.row(e));

                    MatrixXd C_from;
                    VectorXi m_from;
                    gcentroids(X, idx, VectorXi::Constant(1, from), C_from, m_from);
                    C.row(from) = C_from.row(0);
                    m[from] = m_from[0];
                    D.col(from) = distfun(X, C.row(from));
                    if (std::find(changedList.begin(), changedList.end(), from) == changedList.end())
                        changedList.push_back(from);
                }
                std::sort(changedList.begin(), changedList.end());
            }
            changed = Map<VectorXi>(changedList.data(), static_cast<Index>(changedList.size()));
        }

        // Total sum of distances for the current configuration
        totsumD = 0;
        for (qint32 i = 0; i < n; ++i)
            totsumD += D(i, idx[i]);

        // Cycle detection: if objective did not decrease, revert last step
        if (prevtotsumD <= totsumD) {
            idx = previdx;
            MatrixXd C_rev;
            VectorXi m_rev;
            gcentroids(X, idx, changed, C_rev, m_rev);
            for (qint32 i = 0; i < changed.rows(); ++i) {
                C.row(changed[i]) = C_rev.row(i);
                m[changed[i]] = m_rev[i];
            }
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
            d[i] = nanMin(D.row(i), nidx[i]);

        // Determine which points moved
        std::vector<int> movedVec;
        movedVec.reserve(n);
        for (qint32 i = 0; i < n; ++i) {
            if (nidx[i] != previdx[i])
                movedVec.push_back(i);
        }

        // Resolve ties in favor of not moving
        std::vector<int> movedFinal;
        movedFinal.reserve(movedVec.size());
        for (int mi : movedVec) {
            if (D(mi, previdx[mi]) > d[mi])
                movedFinal.push_back(mi);
        }

        if (movedFinal.empty()) {
            converged = true;
            break;
        }

        for (int mi : movedFinal)
            idx[mi] = nidx[mi];

        // Find clusters that gained or lost members
        std::vector<int> tmp;
        tmp.reserve(2 * movedFinal.size());
        for (int mi : movedFinal) {
            tmp.push_back(idx[mi]);
            tmp.push_back(previdx[mi]);
        }
        std::sort(tmp.begin(), tmp.end());
        tmp.erase(std::unique(tmp.begin(), tmp.end()), tmp.end());

        changed.resize(tmp.size());
        for (size_t i = 0; i < tmp.size(); ++i)
            changed[i] = tmp[i];
    }
    return true;
}

//=============================================================================================================

bool KMeans::onlineUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx)
{
    // On binary data the Hamming distance is the city-block distance over p, with the same median centroids
    const bool medianCentroids = m_distance == KMeansDistance::CityBlock || m_distance == KMeansDistance::Hamming;
    const double medianScale = m_distance == KMeansDistance::Hamming ? 1.0 / p : 1.0;
    // Initialize city-block median tracking if needed
    MatrixXd Xmid1, Xmid2;
    if (medianCentroids) {
        Xmid1 = MatrixXd::Zero(k, p);
        Xmid2 = MatrixXd::Zero(k, p);
        for (qint32 i = 0; i < k; ++i) {
            if (m[i] > 0) {
                MatrixXd Xsorted(m[i], p);
                qint32 c = 0;
                for (qint32 j = 0; j < n; ++j)
                    if (idx[j] == i)
                        Xsorted.row(c++) = X.row(j);

                for (qint32 j = 0; j < p; ++j)
                    std::sort(Xsorted.col(j).data(), Xsorted.col(j).data() + Xsorted.rows());

                qint32 nn = static_cast<qint32>(std::floor(0.5 * m[i])) - 1;
                if ((m[i] % 2) == 0) {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn + 1);
                } else if (m[i] > 1) {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn + 2);
                } else {
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

    while (iter < m_iMaxit) {
        // Compute reassignment criterion Del for changed clusters
        if (m_distance == KMeansDistance::SquaredEuclidean) {
            for (qint32 j = 0; j < changed.rows(); ++j) {
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
        } else if (medianCentroids) {
            for (qint32 j = 0; j < changed.rows(); ++j) {
                qint32 i = changed[j];
                if (m(i) % 2 == 0) {
                    MatrixXd ldist = Xmid1.row(i).replicate(n, 1) - X;
                    MatrixXd rdist = X - Xmid2.row(i).replicate(n, 1);
                    VectorXd mbrs = VectorXd::Zero(n);
                    for (qint32 l = 0; l < n; ++l)
                        if (idx[l] == i)
                            mbrs[l] = 1;
                    MatrixXd sgn = ((-2 * mbrs).array() + 1).replicate(1, p);
                    rdist = sgn.array() * rdist.array();
                    ldist = sgn.array() * ldist.array();

                    for (qint32 l = 0; l < n; ++l) {
                        double sum = 0;
                        for (qint32 h = 0; h < p; ++h)
                            sum += std::max(0.0, std::max(rdist(l, h), ldist(l, h)));
                        Del(l, i) = sum * medianScale;
                    }
                } else {
                    Del.col(i) = (X.rowwise() - C.row(i)).array().abs().rowwise().sum() * medianScale;
                }
            }
        } else if (m_distance == KMeansDistance::Cosine || m_distance == KMeansDistance::Correlation) {
            MatrixXd normC = C.array().pow(2).rowwise().sum().sqrt();
            for (qint32 j = 0; j < changed.rows(); ++j) {
                qint32 i = changed[j];
                MatrixXd XCi = X * C.row(i).transpose();

                VectorXi mbrs = VectorXi::Zero(n);
                for (qint32 l = 0; l < n; ++l)
                    if (idx[l] == i)
                        mbrs[l] = 1;

                VectorXi sgn = 1 - 2 * mbrs.array();
                double A = static_cast<double>(m[i]) * normC(i, 0);
                double B = A * A;

                Del.col(i) = 1 + sgn.cast<double>().array() * (A - (B + 2 * sgn.cast<double>().array() * m[i] * XCi.array() + 1).sqrt());
            }
        }

        // Find best move for each point
        previdx = idx;
        prevtotsumD = totsumD;

        VectorXi nidx = VectorXi::Zero(n);
        VectorXd minDel = VectorXd::Zero(n);
        for (qint32 i = 0; i < n; ++i)
            minDel[i] = nanMin(Del.row(i), nidx[i]);

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

        if (movedFinal.empty()) {
            if ((iter == iter1) || nummoved > 0)
                ++iter;
            converged = true;
            break;
        }

        // Pick the next move in cyclic order
        int bestMoved = movedFinal[0];
        int bestDist = ((movedFinal[0] - lastmoved) % n + n) % n;
        for (size_t i = 1; i < movedFinal.size(); ++i) {
            int d_i = ((movedFinal[i] - lastmoved) % n + n) % n;
            if (d_i < bestDist) {
                bestDist = d_i;
                bestMoved = movedFinal[i];
            }
        }
        int movedPt = bestMoved;

        if (movedPt <= lastmoved) {
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
        if (m_distance == KMeansDistance::SquaredEuclidean) {
            C.row(nidx_pt) += (X.row(movedPt) - C.row(nidx_pt)) / m[nidx_pt];
            C.row(oidx) -= (X.row(movedPt) - C.row(oidx)) / m[oidx];
        } else if (medianCentroids) {
            VectorXi onidx(2);
            onidx << oidx, nidx_pt;

            for (qint32 h = 0; h < 2; ++h) {
                qint32 ci = onidx[h];
                MatrixXd Xsorted(m[ci], p);
                qint32 c = 0;
                for (qint32 j = 0; j < n; ++j)
                    if (idx[j] == ci)
                        Xsorted.row(c++) = X.row(j);

                for (qint32 j = 0; j < p; ++j)
                    std::sort(Xsorted.col(j).data(), Xsorted.col(j).data() + Xsorted.rows());

                qint32 nn = static_cast<qint32>(std::floor(0.5 * m[ci])) - 1;
                if ((m[ci] % 2) == 0) {
                    C.row(ci) = 0.5 * (Xsorted.row(nn) + Xsorted.row(nn + 1));
                    Xmid1.row(ci) = Xsorted.row(nn);
                    Xmid2.row(ci) = Xsorted.row(nn + 1);
                } else {
                    C.row(ci) = Xsorted.row(nn + 1);
                    if (m(ci) > 1) {
                        Xmid1.row(ci) = Xsorted.row(nn);
                        Xmid2.row(ci) = Xsorted.row(nn + 2);
                    } else {
                        Xmid1.row(ci) = Xsorted.row(0);
                        Xmid2.row(ci) = Xsorted.row(0);
                    }
                }
            }
        } else if (m_distance == KMeansDistance::Cosine || m_distance == KMeansDistance::Correlation) {
            C.row(nidx_pt).array() += (X.row(movedPt) - C.row(nidx_pt)).array() / m[nidx_pt];
            C.row(oidx).array() += (X.row(movedPt) - C.row(oidx)).array() / m[oidx];
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

    switch (m_distance) {
        case KMeansDistance::SquaredEuclidean:
            for (qint32 i = 0; i < nclusts; ++i)
                D.col(i) = (X.rowwise() - C.row(i)).rowwise().squaredNorm();
            break;

        case KMeansDistance::CityBlock:
            for (qint32 i = 0; i < nclusts; ++i)
                D.col(i) = (X.rowwise() - C.row(i)).cwiseAbs().rowwise().sum();
            break;

        case KMeansDistance::Cosine:
        case KMeansDistance::Correlation: {
            VectorXd normC = C.rowwise().norm();
            for (qint32 i = 0; i < nclusts; ++i) {
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

    for (qint32 i = 0; i < num; ++i) {
        // Collect member indices for cluster clusts[i]
        std::vector<int> members;
        members.reserve(n);
        for (qint32 j = 0; j < index.rows(); ++j)
            if (index[j] == clusts[i])
                members.push_back(j);

        counts[i] = static_cast<qint32>(members.size());
        if (members.empty())
            continue;

        switch (m_distance) {
            case KMeansDistance::SquaredEuclidean:
            case KMeansDistance::Cosine:
            case KMeansDistance::Correlation: {
                centroids.row(i) = RowVectorXd::Zero(p);
                for (int j : members)
                    centroids.row(i) += X.row(j);
                centroids.row(i) /= counts[i];
                break;
            }

            case KMeansDistance::CityBlock:
            case KMeansDistance::Hamming: {
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
        }
    }
}
