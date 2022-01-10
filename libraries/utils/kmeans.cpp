//=============================================================================================================
/**
 * @file     kmeans.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Christoph Dinh. All rights reserved.
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

#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <time.h>

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
: m_sDistance(distance.toStdString())
, m_sStart(start.toStdString())
, m_iReps(replicates)
, m_sEmptyact(emptyact.toStdString())
, m_iMaxit(maxit)
, m_bOnline(online)
, emptyErrCnt(0)
, iter(0)
, k(0)
, n(0)
, p(0)
, totsumD(0)
, prevtotsumD(0)
{
    // Assume one replicate
    if (m_iReps < 1)
        m_iReps = 1;
}

//=============================================================================================================

KMeans::KMeans(std::string distance,
               std::string start,
               qint32 replicates,
               std::string emptyact,
               bool online,
               qint32 maxit)
: m_sDistance(distance)
, m_sStart(start)
, m_iReps(replicates)
, m_sEmptyact(emptyact)
, m_iMaxit(maxit)
, m_bOnline(online)
, emptyErrCnt(0)
, iter(0)
, k(0)
, n(0)
, p(0)
, totsumD(0)
, prevtotsumD(0)
{
    // Assume one replicate
    if (m_iReps < 1)
        m_iReps = 1;
}

//=============================================================================================================

bool KMeans::calculate(MatrixXd X,
                       qint32 kClusters,
                       VectorXi& idx,
                       MatrixXd& C,
                       VectorXd& sumD,
                       MatrixXd& D)
{
    if (kClusters < 1)
        return false;

    //Init random generator
    srand ( time(NULL) );

// n points in p dimensional space
    k = kClusters;
    n = X.rows();
    p = X.cols();

    if(m_sDistance.compare("cosine") == 0)
    {
//        Xnorm = sqrt(sum(X.^2, 2));
//        if any(min(Xnorm) <= eps(max(Xnorm)))
//            error(['Some points have small relative magnitudes, making them ', ...
//                   'effectively zero.\nEither remove those points, or choose a ', ...
//                   'distance other than ''cosine''.']);
//        end
//        X = X ./ Xnorm(:,ones(1,p));
    }
    else if(m_sDistance.compare("correlation")==0)
    {
        X.array() -= (X.rowwise().sum().array() / (double)p).replicate(1,p); //X - X.rowwise().sum();//.repmat(mean(X,2),1,p);
        MatrixXd Xnorm = (X.array().pow(2).rowwise().sum()).sqrt();//sqrt(sum(X.^2, 2));
//        if any(min(Xnorm) <= eps(max(Xnorm)))
//            error(['Some points have small relative standard deviations, making them ', ...
//                   'effectively constant.\nEither remove those points, or choose a ', ...
//                   'distance other than ''correlation''.']);
//        end
        X.array() /= Xnorm.replicate(1,p).array();
    }
//    else if(m_sDistance.compare('hamming')==0)
//    {
//        if ~all(ismember(X(:),[0 1]))
//            error(message('NonbinaryDataForHamm'));
//        end
//    }

    // Start
    RowVectorXd Xmins;
    RowVectorXd Xmaxs;
    if (m_sStart.compare("uniform") == 0)
    {
        if (m_sDistance.compare("hamming") == 0)
        {
            printf("Error: Uniform Start For Hamming\n");
            return false;
        }
        Xmins = X.colwise().minCoeff();
        Xmaxs = X.colwise().maxCoeff();
    }

    //
    // Done with input argument processing, begin clustering
    //
    if (m_bOnline)
    {
        Del = MatrixXd(n,k);
        Del.fill(std::numeric_limits<double>::quiet_NaN());// reassignment criterion
    }

    double totsumDBest = std::numeric_limits<double>::max();
    emptyErrCnt = 0;

    VectorXi idxBest;
    MatrixXd Cbest;
    VectorXd sumDBest;
    MatrixXd Dbest;

    for(qint32 rep = 0; rep < m_iReps; ++rep)
    {
        if (m_sStart.compare("uniform") == 0)
        {
            C = MatrixXd::Zero(k,p);
            for(qint32 i = 0; i < k; ++i)
                for(qint32 j = 0; j < p; ++j)
                    C(i,j) = unifrnd(Xmins[j], Xmaxs[j]);
            // For 'cosine' and 'correlation', these are uniform inside a subset
            // of the unit hypersphere.  Still need to center them for
            // 'correlation'.  (Re)normalization for 'cosine'/'correlation' is
            // done at each iteration.
            if (m_sDistance.compare("correlation") == 0)
                C.array() -= (C.array().rowwise().sum()/p).replicate(1, p).array();
        }
        else if (m_sStart.compare("sample") == 0)
        {
            C = MatrixXd::Zero(k,p);
            for(qint32 i = 0; i < k; ++i)
                C.block(i,0,1,p) = X.block(rand() % n, 0, 1, p);
            // DEBUG
//            C.block(0,0,1,p) = X.block(2, 0, 1, p);
//            C.block(1,0,1,p) = X.block(7, 0, 1, p);
//            C.block(2,0,1,p) = X.block(17, 0, 1, p);
        }
    //    else if (start.compare("cluster") == 0)
    //    {
    //        Xsubset = X(randsample(n,floor(.1*n)),:);
    //        [dum, C] = kmeans(Xsubset, k, varargin{:}, 'start','sample', 'replicates',1);
    //    }
    //    else if (start.compare("numeric") == 0)
    //    {
    //        C = CC(:,:,rep);
    //    }

        // Compute the distance from every point to each cluster centroid and the
        // initial assignment of points to clusters
        D = distfun(X, C);//, 0);
        idx = VectorXi::Zero(D.rows());
        d = VectorXd::Zero(D.rows());

        for(qint32 i = 0; i < D.rows(); ++i)
            d[i] = D.row(i).minCoeff(&idx[i]);

        m = VectorXi::Zero(k);
        for(qint32 i = 0; i < k; ++i)
            for (qint32 j = 0; j < idx.rows(); ++j)
                if(idx[j] == i)
                    ++ m[i];

        try // catch empty cluster errors and move on to next rep
        {
            // Begin phase one:  batch reassignments
            bool converged = batchUpdate(X, C, idx);

            // Begin phase two:  single reassignments
            if (m_bOnline)
                converged = onlineUpdate(X, C, idx);

            if (!converged)
                printf("Failed To Converge during replicate %d\n", rep);

            // Calculate cluster-wise sums of distances
            VectorXi nonempties = VectorXi::Zero(m.rows());
            quint32 count = 0;
            for(qint32 i = 0; i < m.rows(); ++i)
            {
                if(m[i] > 0)
                {
                    nonempties[i] = 1;
                    ++count;
                }
            }
            MatrixXd C_tmp(count,C.cols());
            count = 0;
            for(qint32 i = 0; i < nonempties.rows(); ++i)
            {
                if(nonempties[i])
                {
                    C_tmp.row(count) = C.row(i);
                    ++count;
                }
            }

            MatrixXd D_tmp = distfun(X, C_tmp);//, iter);
            count = 0;
            for(qint32 i = 0; i < nonempties.rows(); ++i)
            {
                if(nonempties[i])
                {
                    D.col(i) = D_tmp.col(count);
                    C.row(i) = C_tmp.row(count);
                    ++count;
                }
            }

            d = VectorXd::Zero(n);
            for(qint32 i = 0; i < n; ++i)
                d[i] += D.array()(idx[i]*n+i);//Colum Major

            sumD = VectorXd::Zero(k);
            for(qint32 i = 0; i < k; ++i)
                for (qint32 j = 0; j < idx.rows(); ++j)
                    if(idx[j] == i)
                        sumD[i] += d[j];

            totsumD = sumD.array().sum();

//            printf("%d iterations, total sum of distances = %f\n", iter, totsumD);

            // Save the best solution so far
            if (totsumD < totsumDBest)
            {
                totsumDBest = totsumD;
                idxBest = idx;
                Cbest = C;
                sumDBest = sumD;
                Dbest = D;
            }
        }
        catch (int e)
        {
            if(e == 0)
            {
                // If an empty cluster error occurred in one of multiple replicates, catch
                // it, warn, and move on to next replicate.  Error only when all replicates
                // fail.  Rethrow an other kind of error.
                if (m_iReps == 1)
                    return false;
                else
                {
                    emptyErrCnt = emptyErrCnt + 1;
//                    printf("Replicate %d terminated: empty cluster created at iteration %d.\n", rep, iter);
                    if (emptyErrCnt == m_iReps)
                    {
//                        error(message('EmptyClusterAllReps'));
                        return false;
                    }

                }
            }
        } // catch

    } // replicates

    // Return the best solution
    idx = idxBest;
    C = Cbest;
    sumD = sumDBest;
    D = Dbest;

//if hadNaNs
//    idx = statinsertnan(wasnan, idx);
//end
    return true;
}

//=============================================================================================================

bool KMeans::batchUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx)
{
    // Every point moved, every cluster will need an update
    qint32 i = 0;
    VectorXi moved(n);
    for(i = 0; i < n; ++i)
        moved[i] = i;

    VectorXi changed(k);
    for(i = 0; i < k; ++i)
        changed[i] = i;

    previdx = VectorXi::Zero(n);

    prevtotsumD = std::numeric_limits<double>::max();//max double

    MatrixXd D = MatrixXd::Zero(X.rows(), k);

    //
    // Begin phase one:  batch reassignments
    //
    iter = 0;
    bool converged = false;
    while(true)
    {
        ++iter;

        // Calculate the new cluster centroids and counts, and update the
        // distance from every point to those new cluster centroids
        MatrixXd C_new;
        VectorXi m_new;
        KMeans::gcentroids(X, idx, changed, C_new, m_new);
        MatrixXd D_new = distfun(X, C_new);//, iter);

        for(qint32 i = 0; i < changed.rows(); ++i)
        {
            C.row(changed[i]) = C_new.row(i);
            D.col(changed[i]) = D_new.col(i);
            m[changed[i]] = m_new[i];
        }

        // Deal with clusters that have just lost all their members
        VectorXi empties = VectorXi::Zero(changed.rows());
        for(qint32 i = 0; i < changed.rows(); ++i)
            if(m(i) == 0)
                empties[i] = 1;

        if (empties.sum() > 0)
        {
            if (m_sEmptyact.compare("error") == 0)
            {
                return converged;
//                throw 0;
            }
            else if (m_sEmptyact.compare("drop") == 0)
            {
    //            // Remove the empty cluster from any further processing
    //            D(:,empties) = NaN;
    //            changed = changed(m(changed) > 0);
    //            warning('Empty cluster created at iteration %d during replicate %d.',iter, rep,);
            }
            else if (m_sEmptyact.compare("singleton") == 0)
            {
    //            warning('Empty cluster created at iteration %d during replicate %d.', iter, rep);

    //            for i = empties
    //                d = D((idx-1)*n + (1:n)'); // use newly updated distances

    //                % Find the point furthest away from its current cluster.
    //                % Take that point out of its cluster and use it to create
    //                % a new singleton cluster to replace the empty one.
    //                [dlarge, lonely] = max(d);
    //                from = idx(lonely); % taking from this cluster
    //                if m(from) < 2
    //                    % In the very unusual event that the cluster had only
    //                    % one member, pick any other non-singleton point.
    //                    from = find(m>1,1,'first');
    //                    lonely = find(idx==from,1,'first');
    //                end
    //                C(i,:) = X(lonely,:);
    //                m(i) = 1;
    //                idx(lonely) = i;
    //                D(:,i) = distfun(X, C(i,:), distance, iter);

    //                % Update clusters from which points are taken
    //                [C(from,:), m(from)] = gcentroids(X, idx, from, distance);
    //                D(:,from) = distfun(X, C(from,:), distance, iter);
    //                changed = unique([changed from]);
    //            end
            }
        }

        // Compute the total sum of distances for the current configuration.
        totsumD = 0;
        for(qint32 i = 0; i < n; ++i)
            totsumD += D.array()(idx[i]*n+i);//Colum Major
        // Test for a cycle: if objective is not decreased, back out
        // the last step and move on to the single update phase
        if(prevtotsumD <= totsumD)
        {
            idx = previdx;
            MatrixXd C_new;
            VectorXi m_new;
            gcentroids(X, idx, changed, C_new, m_new);
            C.block(0,0,k,C.cols()) = C_new;
            m.block(0,0,k,1) = m_new;
            --iter;
            break;
        }

//        printf("%6d\t%6d\t%8d\t%12g\n",iter,1,moved.rows(),totsumD);
        if (iter >= m_iMaxit)
            break;

        // Determine closest cluster for each point and reassign points to clusters
        previdx = idx;
        prevtotsumD = totsumD;

        VectorXi nidx(D.rows());
        for(qint32 i = 0; i < D.rows(); ++i)
            d[i] = D.row(i).minCoeff(&nidx[i]);

        // Determine which points moved
        VectorXi moved = VectorXi::Zero(nidx.rows());
        qint32 count = 0;
        for(qint32 i = 0; i < nidx.rows(); ++i)
        {
            if(nidx[i] != previdx[i])
            {
                moved[count] = i;
                ++count;
            }
        }
        moved.conservativeResize(count);

        if (moved.rows() > 0)
        {
            // Resolve ties in favor of not moving
            VectorXi moved_new = VectorXi::Zero(moved.rows());
            count = 0;
            for(qint32 i = 0; i < moved.rows(); ++i)
            {
                if(D.array()(previdx[moved[i]] * n + moved[i]) > d[moved[i]])
                {
                    moved_new[count] = moved[i];
                    ++count;
                }
            }
            moved_new.conservativeResize(count);
            moved = moved_new;
        }

        if (moved.rows() == 0)
        {
            converged = true;
            break;
        }

        for(qint32 i = 0; i < moved.rows(); ++i)
            if(moved[i] >= 0)
                idx[ moved[i] ] = nidx[ moved[i] ];

        // Find clusters that gained or lost members
        std::vector<int> tmp;
        for(qint32 i = 0; i < moved.rows(); ++i)
            tmp.push_back(idx[moved[i]]);
        for(qint32 i = 0; i < moved.rows(); ++i)
            tmp.push_back(previdx[moved[i]]);

        std::sort(tmp.begin(),tmp.end());

        std::vector<int>::iterator it;
        it = std::unique(tmp.begin(),tmp.end());
        tmp.resize( it - tmp.begin() );

        changed.conservativeResize(tmp.size());

        for(quint32 i = 0; i < tmp.size(); ++i)
            changed[i] = tmp[i];
    } // phase one
    return converged;
} // nested function

//=============================================================================================================

bool KMeans::onlineUpdate(const MatrixXd& X, MatrixXd& C, VectorXi& idx)
{
    // Initialize some cluster information prior to phase two
    MatrixXd Xmid1;
    MatrixXd Xmid2;
    if (m_sDistance.compare("cityblock") == 0)
    {
        Xmid1 = MatrixXd::Zero(k,p);
        Xmid2 = MatrixXd::Zero(k,p);
        for(qint32 i = 0; i < k; ++i)
        {
            if (m[i] > 0)
            {
                // Separate out sorted coords for points in i'th cluster,
                // and save values above and below median, component-wise
                MatrixXd Xsorted(m[i],p);
                qint32 c = 0;
                for(qint32 j = 0; j < idx.rows(); ++j)
                {
                    if(idx[j] == i)
                    {
                        Xsorted.row(c) = X.row(j);
                        ++c;
                    }
                }
                for(qint32 j = 0; j < Xsorted.cols(); ++j)
                    std::sort(Xsorted.col(j).data(),Xsorted.col(j).data()+Xsorted.rows());

                qint32 nn = floor(0.5*m[i])-1;
                if ((m[i] % 2) == 0)
                {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn+1);
                }
                else if (m[i] > 1)
                {
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn+2);
                }
                else
                {
                    Xmid1.row(i) = Xsorted.row(0);
                    Xmid2.row(i) = Xsorted.row(0);
                }
            }
        }
    }
    else if (m_sDistance.compare("hamming") == 0)
    {
//    Xsum = zeros(k,p);
//    for i = 1:k
//        if m(i) > 0
//            % Sum coords for points in i'th cluster, component-wise
//            Xsum(i,:) = sum(X(idx==i,:), 1);
//        end
//    end
    }

    //
    // Begin phase two:  single reassignments
    //
    VectorXi changed = VectorXi(m.rows());
    qint32 count = 0;
    for(qint32 i = 0; i < m.rows(); ++i)
    {
        if(m[i] > 0)
        {
            changed[count] = i;
            ++count;
        }
    }
    changed.conservativeResize(count);

    qint32 lastmoved = 0;
    qint32 nummoved = 0;
    qint32 iter1 = iter;
    bool converged = false;
    while (iter < m_iMaxit)
    {
        // Calculate distances to each cluster from each point, and the
        // potential change in total sum of errors for adding or removing
        // each point from each cluster.  Clusters that have not changed
        // membership need not be updated.
        //
        // Singleton clusters are a special case for the sum of dists
        // calculation.  Removing their only point is never best, so the
        // reassignment criterion had better guarantee that a singleton
        // point will stay in its own cluster.  Happily, we get
        // Del(i,idx(i)) == 0 automatically for them.

        if (m_sDistance.compare("sqeuclidean") == 0)
        {
            for(qint32 j = 0; j < changed.rows(); ++j)
            {
                qint32 i = changed[j];
                VectorXi mbrs = VectorXi::Zero(idx.rows());
                for(qint32 l = 0; l < idx.rows(); ++l)
                    if(idx[l] == i)
                        mbrs[l] = 1;

                VectorXi sgn = 1 - 2 * mbrs.array(); // -1 for members, 1 for nonmembers

                if (m[i] == 1)
                    for(qint32 l = 0; l < mbrs.rows(); ++l)
                        if(mbrs[l])
                            sgn[l] = 0; // prevent divide-by-zero for singleton mbrs

                Del.col(i) = ((double)m[i] / ((double)m[i] + sgn.cast<double>().array()));

                Del.col(i).array() *= (X - C.row(i).replicate(n,1)).array().pow(2).rowwise().sum().array();
            }
        }
        else if (m_sDistance.compare("cityblock") == 0)
        {
            for(qint32 j = 0; j < changed.rows(); ++j)
            {
                qint32 i = changed[j];
                if (m(i) % 2 == 0) // this will never catch singleton clusters
                {
                    MatrixXd ldist = Xmid1.row(i).replicate(n,1) - X;
                    MatrixXd rdist = X - Xmid2.row(i).replicate(n,1);
                    VectorXd mbrs = VectorXd::Zero(idx.rows());

                    for(qint32 l = 0; l < idx.rows(); ++l)
                        if(idx[l] == i)
                            mbrs[l] = 1;
                    MatrixXd sgn = ((-2*mbrs).array() + 1).replicate(1, p); // -1 for members, 1 for nonmembers
                    rdist = sgn.array()*rdist.array(); ldist = sgn.array()*ldist.array();

                    for(qint32 l = 0; l < idx.rows(); ++l)
                    {
                        double sum = 0;
                        for(qint32 h = 0; h < rdist.cols(); ++h)
                            sum += rdist(l,h) > ldist(l,h) ? rdist(l,h) < 0 ? 0 : rdist(l,h) : ldist(l,h) < 0 ? 0 : ldist(l,h);
                        Del(l,i) = sum;
                    }
                }
                else
                    Del.col(i) = ((X - C.row(i).replicate(n,1)).array().abs()).rowwise().sum();
            }
        }
        else if (m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
        {
            // The points are normalized, centroids are not, so normalize them
            MatrixXd normC = C.array().pow(2).rowwise().sum().sqrt();
//            if any(normC < eps(class(normC))) % small relative to unit-length data points
//                error('Zero cluster centroid created at iteration %d during replicate %d.',iter, rep);
//            end
            // This can be done without a loop, but the loop saves memory allocations
            MatrixXd XCi;
            qint32 i;
            for(qint32 j = 0; j < changed.rows(); ++j)
            {
                i = changed[j];
                XCi = X * C.row(i).transpose();

                VectorXi mbrs = VectorXi::Zero(idx.rows());
                for(qint32 l = 0; l < idx.rows(); ++l)
                    if(idx[l] == i)
                        mbrs[l] = 1;

                VectorXi sgn = 1 - 2 * mbrs.array(); // -1 for members, 1 for nonmembers

                double A = (double)m[i] * normC(i,0);
                double B = pow(((double)m[i] * normC(i,0)),2);

                Del.col(i) = 1 + sgn.cast<double>().array()*
                        (A - (B + 2 * sgn.cast<double>().array() * m[i] * XCi.array() + 1).sqrt());

                std::cout << "Del.col(i)\n" << Del.col(i) << std::endl;

//                Del(:,i) = 1 + sgn .*...
//                      (m(i).*normC(i) - sqrt((m(i).*normC(i)).^2 + 2.*sgn.*m(i).*XCi + 1));
            }
        }
        else if (m_sDistance.compare("hamming") == 0)
        {
//            for i = changed
//                if mod(m(i),2) == 0 % this will never catch singleton clusters
//                    % coords with an unequal number of 0s and 1s have a
//                    % different contribution than coords with an equal
//                    % number
//                    unequal01 = find(2*Xsum(i,:) ~= m(i));
//                    numequal01 = p - length(unequal01);
//                    mbrs = (idx == i);
//                    Di = abs(X(:,unequal01) - C(repmat(i,n,1),unequal01));
//                    Del(:,i) = (sum(Di, 2) + mbrs*numequal01) / p;
//                else
//                    Del(:,i) = sum(abs(X - C(repmat(i,n,1),:)), 2) / p;
//                end
//            end
        }

        // Determine best possible move, if any, for each point.  Next we
        // will pick one from those that actually did move.
        previdx = idx;
        prevtotsumD = totsumD;

        VectorXi nidx = VectorXi::Zero(Del.rows());
        VectorXd minDel = VectorXd::Zero(Del.rows());

        for(qint32 i = 0; i < Del.rows(); ++i)
            minDel[i] = Del.row(i).minCoeff(&nidx[i]);

        VectorXi moved = VectorXi::Zero(previdx.rows());
        qint32 count = 0;
        for(qint32 i = 0; i < moved.rows(); ++i)
        {
            if(previdx[i] != nidx[i])
            {
                moved[count] = i;
                ++count;
            }
        }
        moved.conservativeResize(count);

        if (moved.sum() > 0)
        {
            // Resolve ties in favor of not moving
            VectorXi moved_new = VectorXi::Zero(moved.rows());
            count = 0;
            for(qint32 i = 0; i < moved.rows(); ++i)
            {
                if ( Del.array()(previdx[moved(i)]*n + moved(i)) > minDel(moved(i)))
                {
                    moved_new[count] = moved[i];
                    ++count;
                }
            }
            moved_new.conservativeResize(count);
            moved = moved_new;
        }

        if (moved.rows() <= 0)
        {
            // Count an iteration if phase 2 did nothing at all, or if we're
            // in the middle of a pass through all the points
            if ((iter == iter1) || nummoved > 0)
            {
                ++iter;

//                printf("%6d\t%6d\t%8d\t%12g\n",iter,2,nummoved,totsumD);
            }
            converged = true;
            break;
        }

        // Pick the next move in cyclic order
        VectorXi moved_new(moved.rows());
        for(qint32 i = 0; i < moved.rows(); ++i)
            moved_new[i] = ((moved[i] - lastmoved) % n) + lastmoved;

        moved[0] = moved_new.minCoeff() % n;//+1
        moved.conservativeResize(1);

        // If we've gone once through all the points, that's an iteration
        if (moved[0] <= lastmoved)
        {
            ++iter;
//            printf("%6d\t%6d\t%8d\t%12g\n",iter,2,nummoved,totsumD);
            if(iter >= m_iMaxit)
                break;
            nummoved = 0;
        }
        ++nummoved;
        lastmoved = moved[0];

        qint32 oidx = idx(moved[0]);
        nidx[0] = nidx(moved[0]);
        nidx.conservativeResize(1);
        totsumD += Del(moved[0],nidx[0]) - Del(moved[0],oidx);

        // Update the cluster index vector, and the old and new cluster
        // counts and centroids
        idx[ moved[0] ] = nidx[0];
        m( nidx[0] ) = m( nidx[0] ) + 1;
        m( oidx ) = m( oidx ) - 1;

        if (m_sDistance.compare("sqeuclidean") == 0)
        {
            C.row(nidx[0]) = C.row(nidx[0]).array() + (X.row(moved[0]) - C.row(nidx[0])).array() / m[nidx[0]];
            C.row(oidx) = C.row(oidx).array() - (X.row(moved[0]) - C.row(oidx)).array() / m[oidx];
        }
        else if (m_sDistance.compare("cityblock") == 0)
        {
            VectorXi onidx(2);
            onidx << oidx, nidx[0];//ToDo always right?

            qint32 i;
            for(qint32 h = 0; h < 2; ++h)
            {
                i = onidx[h];
                // Separate out sorted coords for points in each cluster.
                // New centroid is the coord median, save values above and
                // below median.  All done component-wise.
                MatrixXd Xsorted(m[i],p);
                qint32 c = 0;
                for(qint32 j = 0; j < idx.rows(); ++j)
                {
                    if(idx[j] == i)
                    {
                        Xsorted.row(c) = X.row(j);
                        ++c;
                    }
                }
                for(qint32 j = 0; j < Xsorted.cols(); ++j)
                    std::sort(Xsorted.col(j).data(),Xsorted.col(j).data()+Xsorted.rows());

                qint32 nn = floor(0.5*m[i])-1;
                if ((m[i] % 2) == 0)
                {
                    C.row(i) = 0.5 * (Xsorted.row(nn) + Xsorted.row(nn+1));
                    Xmid1.row(i) = Xsorted.row(nn);
                    Xmid2.row(i) = Xsorted.row(nn+1);
                }
                else
                {
                    C.row(i) = Xsorted.row(nn+1);
                    if (m(i) > 1)
                    {
                        Xmid1.row(i) = Xsorted.row(nn);
                        Xmid2.row(i) = Xsorted.row(nn+2);
                    }
                    else
                    {
                        Xmid1.row(i) = Xsorted.row(0);
                        Xmid2.row(i) = Xsorted.row(0);
                    }
                }
            }
        }
        else if (m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
        {
            C.row(nidx[0]).array() += (X.row(moved[0]) - C.row(nidx[0])).array() / m[nidx[0]];
            C.row(oidx).array() += (X.row(moved[0]) - C.row(oidx)).array() / m[oidx];
        }
        else if (m_sDistance.compare("hamming") == 0)
        {
//                % Update summed coords for points in each cluster.  New
//                % centroid is the coord median.  All done component-wise.
//                Xsum(nidx,:) = Xsum(nidx,:) + X(moved,:);
//                Xsum(oidx,:) = Xsum(oidx,:) - X(moved,:);
//                C(nidx,:) = .5*sign(2*Xsum(nidx,:) - m(nidx)) + .5;
//                C(oidx,:) = .5*sign(2*Xsum(oidx,:) - m(oidx)) + .5;
        }

        VectorXi sorted_onidx(1+nidx.rows());
        sorted_onidx << oidx, nidx;
        std::sort(sorted_onidx.data(), sorted_onidx.data()+sorted_onidx.rows());
        changed = sorted_onidx;
    } // phase two

    return converged;
} // nested function

//=============================================================================================================
//DISTFUN Calculate point to cluster centroid distances.
MatrixXd KMeans::distfun(const MatrixXd& X, MatrixXd& C)//, qint32 iter)
{
    MatrixXd D = MatrixXd::Zero(n,C.rows());
    qint32 nclusts = C.rows();

    if (m_sDistance.compare("sqeuclidean") == 0)
    {
        for(qint32 i = 0; i < nclusts; ++i)
        {
            D.col(i) = (X.col(0).array() - C(i,0)).pow(2);

            for(qint32 j = 1; j < p; ++j)
                D.col(i) = D.col(i).array() + (X.col(j).array() - C(i,j)).pow(2);
        }
    }
    else if (m_sDistance.compare("cityblock") == 0)
    {
        for(qint32 i = 0; i < nclusts; ++i)
        {
            D.col(i) = (X.col(0).array() - C(i,0)).array().abs();
            for(qint32 j = 1; j < p; ++j)
            {
                D.col(i).array() += (X.col(j).array() - C(i,j)).array().abs();
            }
        }
    }
    else if (m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
    {
        // The points are normalized, centroids are not, so normalize them
        MatrixXd normC = C.array().pow(2).rowwise().sum().sqrt();
//        if any(normC < eps(class(normC))) % small relative to unit-length data points
//            error('Zero cluster centroid created at iteration %d.',iter);
        for (qint32 i = 0; i < nclusts; ++i)
        {
            MatrixXd C_tmp = (C.row(i).array() / normC(i,0)).transpose();
            D.col(i) = X * C_tmp;//max(1 - X * (C(i,:)./normC(i))', 0);
            for(qint32 j = 0; j < D.rows(); ++j)
                if(D(j,i) < 0)
                    D(j,i) = 0;
        }
    }
//case 'hamming'
//    for i = 1:nclusts
//        D(:,i) = abs(X(:,1) - C(i,1));
//        for j = 2:p
//            D(:,i) = D(:,i) + abs(X(:,j) - C(i,j));
//        end
//        D(:,i) = D(:,i) / p;
//        % D(:,i) = sum(abs(X - C(repmat(i,n,1),:)), 2) / p;
//    end
//end
    return D;
} // function

//=============================================================================================================
//GCENTROIDS Centroids and counts stratified by group.
void KMeans::gcentroids(const MatrixXd& X, const VectorXi& index, const VectorXi& clusts,
                                          MatrixXd& centroids, VectorXi& counts)
{
    qint32 num = clusts.rows();
    centroids = MatrixXd::Zero(num,p);
    centroids.fill(std::numeric_limits<double>::quiet_NaN());
    counts = VectorXi::Zero(num);

    VectorXi members;

    qint32 c;

    for(qint32 i = 0; i < num; ++i)
    {
        c = 0;
        members = VectorXi::Zero(index.rows());
        for(qint32 j = 0; j < index.rows(); ++j)
        {
            if(index[j] == clusts[i])
            {
                members[c] = j;
                ++c;
            }
        }
        members.conservativeResize(c);
        if (c > 0)
        {
            counts[i] = c;
            if(m_sDistance.compare("sqeuclidean") == 0)
            {
                //Initialize
                if(members.rows() > 0)
                    centroids.row(i) = RowVectorXd::Zero(centroids.cols());

                for(qint32 j = 0; j < members.rows(); ++j)
                    centroids.row(i).array() += X.row(members[j]).array() / counts[i];
            }
            else if(m_sDistance.compare("cityblock") == 0)
            {
                // Separate out sorted coords for points in i'th cluster,
                // and use to compute a fast median, component-wise
                MatrixXd Xsorted(counts[i],p);
                c = 0;

                for(qint32 j = 0; j < index.rows(); ++j)
                {
                    if(index[j] == clusts[i])
                    {
                        Xsorted.row(c) = X.row(j);
                        ++c;
                    }
                }

                for(qint32 j = 0; j < Xsorted.cols(); ++j)
                    std::sort(Xsorted.col(j).data(),Xsorted.col(j).data()+Xsorted.rows());

                qint32 nn = floor(0.5*(counts(i)))-1;
                if (counts[i] % 2 == 0)
                    centroids.row(i) = .5 * (Xsorted.row(nn) + Xsorted.row(nn+1));
                else
                    centroids.row(i) = Xsorted.row(nn+1);
            }
            else if(m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
            {
                for(qint32 j = 0; j < members.rows(); ++j)
                    centroids.row(i).array() += X.row(members[j]).array() / counts[i]; // unnormalized
            }
//            else if(m_sDistance.compare("hamming") == 0)
//            {
//                % Compute a fast median for binary data, component-wise
//                centroids(i,:) = .5*sign(2*sum(X(members,:), 1) - counts(i)) + .5;
//            }
        }
    }
}// function

//=============================================================================================================

double KMeans::unifrnd(double a, double b)
{
    if (a > b)
        return std::numeric_limits<double>::quiet_NaN();

    double a2 = a/2.0;
    double b2 = b/2.0;
    double mu = a2+b2;
    double sig = b2-a2;

    double r = mu + sig * (2.0* (rand() % 1000)/1000 -1.0);

    return r;
}
