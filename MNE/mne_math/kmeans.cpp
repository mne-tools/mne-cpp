//=============================================================================================================
/**
* @file     kmeans.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
* @version  1.0
* @date     July, 2012
*
* @section  LICENSE
*
* Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that
* the following conditions are met:
*     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
*       following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*       the following disclaimer in the documentation and/or other materials provided with the distribution.
*     * Neither the name of the Massachusetts General Hospital nor the names of its contributors may be used
*       to endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
* PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MASSACHUSETTS GENERAL HOSPITAL BE LIABLE FOR ANY DIRECT,
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*
* @brief    ToDo Documentation...
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "kmeans.h"


//*************************************************************************************************************
//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <math.h>
#include <iostream>
#include <algorithm>
#include <vector>


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <QDebug>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNE_MATHLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

//KMeans::KMeans(MatrixXd &ddata, MatrixXd &mmeans)
//: nn(ddata.rows())
//, mm(ddata.cols())
//, kk(mmeans.rows())
//, data(ddata)
//, means(mmeans)
//, assign(VectorXi::Zero(nn))
//, count(VectorXi::Zero(kk))
//{
//    estep();
//    mstep();
//}


////*************************************************************************************************************

//int KMeans::estep() {
//    int k,m,n,kmin;
//    double dmin,d;
//    nchg = 0;
//    for (k=0;k<kk;k++)
//        count[k] = 0;
//    for (n=0;n<nn;n++)
//    {
//        dmin = 9.99e99;
//        for (k=0;k<kk;k++)
//        {
//            for (d=0.,m=0; m<mm; m++)
//                d += sqrt(abs(data(n,m) - means(k,m)));
//            if (d < dmin)
//            {
//                dmin = d;
//                kmin = k;
//            }
//        }
//        if (kmin != assign[n])
//            nchg++;
//        assign[n] = kmin;
//        count[kmin]++;
//    }
//    return nchg;
//}


////*************************************************************************************************************

//void KMeans::mstep()
//{
//    int n,k,m;
//    for (k=0;k<kk;k++)
//        for (m=0;m<mm;m++)
//            means(k, m) = 0.;

//    for (n=0; n<nn; n++)
//        for (m=0;m<mm;m++)
//            means(assign[n],m) += data(n,m);

//    for (k=0;k<kk;k++)
//    {
//        if (count[k] > 0)
//            for (m=0;m<mm;m++)
//                means(k, m) /= count[k];
//    }
//}



KMeans::KMeans(QString &distance, QString &start, qint32 replicates, QString& emptyact, qint32 maxit, bool online)
: m_sDistance(distance)
, m_sStart(start)
, m_iReps(replicates)
, m_sEmptyact(emptyact)
, m_iMaxit(maxit)
, m_bOnline(online)
{
    // Assume one replicate
    if (m_iReps < 1)
        m_iReps = 1;
}





//*************************************************************************************************************

void KMeans::calculate(    MatrixXd X, qint32 kClusters,
                            VectorXi& idx, MatrixXd& C, VectorXd& sumD, MatrixXd& D)
{

// n points in p dimensional space
    k = kClusters;
    n = X.rows();
    p = X.cols();

//pnames = {   'distance'  'start' 'replicates' 'emptyaction' 'onlinephase' 'options' 'maxiter' 'display'};
//dflts =  {'sqeuclidean' 'sample'          []         'error'         'on'        []        []        []};

//    if(distance.compare('cosine') == 0)
//    {
//        Xnorm = sqrt(sum(X.^2, 2));
//        if any(min(Xnorm) <= eps(max(Xnorm)))
//            error(['Some points have small relative magnitudes, making them ', ...
//                   'effectively zero.\nEither remove those points, or choose a ', ...
//                   'distance other than ''cosine''.']);
//        end
//        X = X ./ Xnorm(:,ones(1,p));
//    }
//    else if(distance.compare('correlation')==0)
//    {
//        X = X - repmat(mean(X,2),1,p);
//        Xnorm = sqrt(sum(X.^2, 2));
//        if any(min(Xnorm) <= eps(max(Xnorm)))
//            error(['Some points have small relative standard deviations, making them ', ...
//                   'effectively constant.\nEither remove those points, or choose a ', ...
//                   'distance other than ''correlation''.']);
//        end
//        X = X ./ Xnorm(:,ones(1,p));
//    }
//    else if(distance.compare('hamming')==0)
//    {
//        if ~all(ismember(X(:),[0 1]))
//            error(message('NonbinaryDataForHamm'));
//        end
//    }

//# Start
//    if (start.compare('uniform') == 0)
//    {
//        if (distance.compare('hamming') == 0)
//        {
//            printf('Error:kmeans:UniformStartForHamm\n');
//            return;
//        }
//        Xmins = min(X,[],1);
//        Xmaxs = max(X,[],1);
//    }

//
// Done with input argument processing, begin clustering
//

    //dispfmt = '%6d\t%6d\t%8d\t%12g';
    if (m_bOnline)
    {
        Del = MatrixXd(n,k);
        Del.fill(std::numeric_limits<double>::quiet_NaN());// reassignment criterion
    }

    double totsumDBest = std::numeric_limits<double>::max();
    //emptyErrCnt = 0;

    VectorXi idxBest;
    MatrixXd Cbest;
    VectorXd sumDBest;
    MatrixXd Dbest;

    for(qint32 rep = 0; rep < m_iReps; ++rep)
    {
    //    if (start.compare('uniform') == 0)
    //    {
    //        C = unifrnd(Xmins(ones(k,1),:), Xmaxs(ones(k,1),:));
    //        % For 'cosine' and 'correlation', these are uniform inside a subset
    //        % of the unit hypersphere.  Still need to center them for
    //        % 'correlation'.  (Re)normalization for 'cosine'/'correlation' is
    //        % done at each iteration.
    //        if isequal(distance, 'correlation')
    //            C = C - repmat(mean(C,2),1,p);
    //        end
    //        if isa(X,'single')
    //            C = single(C);
    //        end
    //    }
        /*else*/ if (m_sStart.compare("sample") == 0)
        {
            C = MatrixXd::Zero(k,p);
            for(qint32 i = 0; i < k; ++i)
                C.block(i,0,1,p) = X.block(rand() % n, 0, 1, p);
//            C.block(0,0,1,p) = X.block(2, 0, 1, p);
//            C.block(1,0,1,p) = X.block(7, 0, 1, p);
//            std::cout << "C" << std::endl << C << std::endl;
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
        D = distfun(X, C, 0);
        idx = VectorXi::Zero(D.rows());
        d = VectorXd::Zero(D.rows());

    //    [d, idx] = min(D, [], 2);
        for(qint32 i = 0; i < D.rows(); ++i)
            d[i] = D.row(i).minCoeff(&idx[i]);

//        std::cout << "d" << std::endl << d << std::endl;

//        std::cout << "idx" << std::endl << idx << std::endl;

        m = VectorXi::Zero(k);
        for(qint32 i = 0; i < k; ++i)
            for (qint32 j = 0; j < idx.rows(); ++j)
                if(idx[j] == i)
                    ++ m[i];

//        std::cout << "m" << std::endl << m << std::endl;


    //    try % catch empty cluster errors and move on to next rep

        // Begin phase one:  batch reassignments
        bool converged = batchUpdate(X, C, idx);

        qDebug() << converged;
        std::cout << "C batch" << std::endl << C << std::endl;
//        std::cout << "idx" << std::endl << idx << std::endl;


        // Begin phase two:  single reassignments
        if (m_bOnline)
            converged = onlineUpdate(X, C, idx);

        qDebug() << converged;
        std::cout << "C online" << std::endl << C << std::endl;
//        std::cout << "idx" << std::endl << idx << std::endl;


        if (!converged)
            printf("Failed To Converge during replicate %d\n", rep);

        // Calculate cluster-wise sums of distances
        VectorXi nonempties = VectorXi::Zero(m.rows());
        quint32 count = 0;
        for(qint32 i = 0; i < m.rows(); ++i)
        {
            if(m[i] > 0)//find(m>0);
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

        MatrixXd D_tmp = distfun(X, C_tmp, iter);
    //    D(:,nonempties) = distfun(X, C(nonempties,:), distance, iter);
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

        for(qint32 i = 0; i < n; ++i)
            d[i] += D.array()(idx[i]*n+i);//Colum Major

        sumD = VectorXd::Zero(k);
        for(qint32 i = 0; i < k; ++i)
            for (qint32 j = 0; j < idx.rows(); ++j)
                if(idx[j] == i)
                    ++ sumD[i];

        totsumD = sumD.array().sum();

//        d = D((idx-1)*n + (1:n)');
//        sumD = accumarray(idx,d,[k,1]);
//        totsumD = sum(sumD);

//        if display > 1 % 'final' or 'iter'
//            disp(sprintf('%d iterations, total sum of distances = %g',iter,totsumD));
//        end

        // Save the best solution so far
        if (totsumD < totsumDBest)
        {
            totsumDBest = totsumD;
            idxBest = idx;
            Cbest = C;
            sumDBest = sumD;
//            if nargout > 3
                Dbest = D;
//            end
        }

//        % If an empty cluster error occurred in one of multiple replicates, catch
//        % it, warn, and move on to next replicate.  Error only when all replicates
//        % fail.  Rethrow an other kind of error.
//        catch ME
//            if reps == 1 || ~isequal(ME.identifier,'EmptyCluster')
//                rethrow(ME);
//            else
//                emptyErrCnt = emptyErrCnt + 1;
//                warning('Replicate %d terminated: empty cluster created at iteration %d.',rep,iter);
//                if emptyErrCnt == reps
//                    error(message('EmptyClusterAllReps'));
//                end
//            end
//        end % catch

    } // replicates

    // Return the best solution
    idx = idxBest;
    C = Cbest;
    sumD = sumDBest;
    //if nargout > 3
    D = Dbest;
    //end

//if hadNaNs
//    idx = statinsertnan(wasnan, idx);
//end
}


//*************************************************************************************************************

bool KMeans::batchUpdate(MatrixXd& X, MatrixXd& C, VectorXi& idx)
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

    //
    // Begin phase one:  batch reassignments
    //

    iter = 0;
    bool converged = false;
    while(true)
    {
        ++iter;

//        qDebug() << "iter " << iter;

        // Calculate the new cluster centroids and counts, and update the
        // distance from every point to those new cluster centroids
        MatrixXd C_new;
        VectorXi m_new;
        KMeans::gcentroids(X, idx, changed, C_new, m_new);
        C.block(0,0,k,C.cols()) = C_new;
        m.block(0,0,k,1) = m_new;

//        std::cout << "C" << std::endl << C << std::endl;
//        std::cout << "m" << std::endl << m << std::endl;

        MatrixXd C_changed = C.block(0,0,k,C.cols());

        MatrixXd D = distfun(X, C_changed, iter);

        // Deal with clusters that have just lost all their members
        VectorXi empties = VectorXi::Zero(changed.rows());
        for(qint32 i = 0; i < changed.rows(); ++i)
            if(m(i) == 0)
                empties[i] = 1;

        if (empties.sum() > 0)
        {
            if (m_sEmptyact.compare("error") == 0)
            {
                printf("Error: Empty cluster created at iteration %d\n", iter);// during replicate %d.", iter, rep);
                return false;
            }
            else if (m_sEmptyact.compare("drop") == 0)
            {
    //            % Remove the empty cluster from any further processing
    //            D(:,empties) = NaN;
    //            changed = changed(m(changed) > 0);
    //            warning('Empty cluster created at iteration %d during replicate %d.',iter, rep,);
            }
            else if (m_sEmptyact.compare("singleton") == 0)
            {
    //            warning('Empty cluster created at iteration %d during replicate %d.', iter, rep);

    //            for i = empties
    //                d = D((idx-1)*n + (1:n)'); % use newly updated distances

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
    //        [C(changed,:), m(changed)] = gcentroids(X, idx, changed, distance);
            --iter;
            break;
        }
    //    if display > 2 % 'iter'
    //        disp(sprintf(dispfmt,iter,1,length(moved),totsumD));
    //    end
        if (iter >= m_iMaxit)
            break;

        // Determine closest cluster for each point and reassign points to clusters
        previdx = idx;
        prevtotsumD = totsumD;

    //    [d, nidx] = min(D, [], 2);
        VectorXi nidx(D.rows());
        for(qint32 i = 0; i < D.rows(); ++i)
            d[i] = D.row(i).minCoeff(&nidx[i]);

        // Determine which points moved
        //ToDo this can be spead up
        VectorXi moved = VectorXi::Zero(nidx.rows());
//        std::vector<qint32> moved;
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

//        std::cout << "changed" << std::endl << changed << std::endl;
//        changed = unique([idx(moved); previdx(moved)])';

    } // phase one
    return converged;
} // nested function



//*************************************************************************************************************

bool KMeans::onlineUpdate(MatrixXd& X, MatrixXd& C, VectorXi& idx)
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
//                Xsorted = sort(X(idx==i,:),1);

                MatrixXd Xsorted(m[i],p);
                qint32 c = 0;
//                std::cout << "X" << std::endl << X << std::endl;
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

//                std::cout << "Xsorted" << std::endl << Xsorted << std::endl;

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

//                std::cout << "Xmid1" << std::endl << Xmid1 << std::endl;
//                std::cout << "Xmid2" << std::endl << Xmid2 << std::endl;

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
//            for i = changed
//                mbrs = (idx == i);
//                sgn = 1 - 2*mbrs; % -1 for members, 1 for nonmembers
//                if m(i) == 1
//                    sgn(mbrs) = 0; % prevent divide-by-zero for singleton mbrs
//                end
//                Del(:,i) = (m(i) ./ (m(i) + sgn)) .* sum((X - C(repmat(i,n,1),:)).^2, 2);
//            end
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
//                    Del(:,i) = sum(max(0, max(sgn.*rdist, sgn.*ldist)), 2);

                }
                else
                {
//                    std::cout << ((X - C.row(i).replicate(n,1)).array().abs()).rowwise().sum();
                    Del.col(i) = ((X - C.row(i).replicate(n,1)).array().abs()).rowwise().sum();
//                    Del(:,i) = sum(abs(X - C(repmat(i,n,1),:)), 2);
                }
            }
        }
        else if (m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
        {
//            % The points are normalized, centroids are not, so normalize them
//            normC = sqrt(sum(C.^2, 2));
//            if any(normC < eps(class(normC))) % small relative to unit-length data points
//                error('Zero cluster centroid created at iteration %d during replicate %d.',iter, rep);
//            end
//            % This can be done without a loop, but the loop saves memory allocations
//            for i = changed
//                XCi = X * C(i,:)';
//                mbrs = (idx == i);
//                sgn = 1 - 2*mbrs; % -1 for members, 1 for nonmembers
//                Del(:,i) = 1 + sgn .*...
//                      (m(i).*normC(i) - sqrt((m(i).*normC(i)).^2 + 2.*sgn.*m(i).*XCi + 1));
//            end
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


    //    [minDel, nidx] = min(Del, [], 2);
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

    //    moved = find(previdx ~= nidx);
        if (moved.sum() > 0) //~isempty(moved)
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

        std::cout << "moved" << std::endl << moved << std::endl;

        if (moved.rows() <= 0)
        {
            // Count an iteration if phase 2 did nothing at all, or if we're
            // in the middle of a pass through all the points
            if ((iter == iter1) || nummoved > 0)
            {
                ++iter;
//                if display > 2 % 'iter'
//                    disp(sprintf(dispfmt,iter,2,nummoved,totsumD));
//                end
            }
            converged = true;
            break;
        }


//        std::cout << "moved" << std::endl << moved << std::endl;
//        std::cout << "lastmoved" << std::endl << lastmoved << std::endl;


        // Pick the next move in cyclic order
        VectorXi moved_new(moved.rows());
        for(qint32 i = 0; i < moved.rows(); ++i)
            moved_new[i] = ((moved[i] - lastmoved) % n) + lastmoved;

        moved[0] = moved_new.minCoeff() % n;//+1
        moved.conservativeResize(1);


//        std::cout << "moved" << std::endl << moved << std::endl;


        // If we've gone once through all the points, that's an iteration
        if (moved[0] <= lastmoved)
        {
            ++iter;
//            if display > 2 % 'iter'
//                disp(sprintf(dispfmt,iter,2,nummoved,totsumD));
//            end
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
//            C(nidx,:) = C(nidx,:) + (X(moved,:) - C(nidx,:)) / m(nidx);
//            C(oidx,:) = C(oidx,:) - (X(moved,:) - C(oidx,:)) / m(oidx);
        }
        else if (m_sDistance.compare("cityblock") == 0)
        {
            VectorXi onidx(1 + nidx.rows());
            onidx << oidx, nidx;

            qint32 i;
            for(qint32 h = 0; h < 2; ++h)
            {
                i = onidx[h];
                // Separate out sorted coords for points in each cluster.
                // New centroid is the coord median, save values above and
                // below median.  All done component-wise.
//                Xsorted = sort(X(idx==i,:),1);

                MatrixXd Xsorted(m[i],p);
                qint32 c = 0;
//                std::cout << "X" << std::endl << X << std::endl;
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
//                    Xmid(i,:,1:2) = Xsorted([nn, nn+1],:)';
                }
                else
                {
                    C.row(i) = Xsorted.row(nn+1);
                    if (m(i) > 1)
                    {
                        Xmid1.row(i) = Xsorted.row(nn);
                        Xmid2.row(i) = Xsorted.row(nn+2);
//                        Xmid(i,:,1:2) = Xsorted([nn, nn+2],:)';
                    }
                    else
                    {
                        Xmid1.row(i) = Xsorted.row(0);
                        Xmid2.row(i) = Xsorted.row(0);
//                        Xmid(i,:,1:2) = Xsorted([1, 1],:)';
                    }
                }
            }
        }
        else if (m_sDistance.compare("cosine") == 0 || m_sDistance.compare("correlation") == 0)
        {
//            C(nidx,:) = C(nidx,:) + (X(moved,:) - C(nidx,:)) / m(nidx);
//            C(oidx,:) = C(oidx,:) - (X(moved,:) - C(oidx,:)) / m(oidx);
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

//        changed = sort([oidx nidx]);
    } // phase two

    return converged;

} // nested function


//*************************************************************************************************************
//DISTFUN Calculate point to cluster centroid distances.
MatrixXd KMeans::distfun(MatrixXd& X, MatrixXd& C, qint32 iter)
{
    MatrixXd D = MatrixXd::Zero(n,C.rows());
    qint32 nclusts = C.rows();

//    qDebug() << m_sDistance;
//    qDebug() << "D: " << D.rows() << "x" << D.cols();
//    qDebug() << "X: " << X.rows() << "x" << X.cols();
//    qDebug() << "C: " << C.rows() << "x" << C.cols();

//    std::cout << "C" << std::endl << C << std::endl;


//    if (m_sDistance.compare("sqeuclidean") == 0)
//    {
//    for i = 1:nclusts
//        D(:,i) = (X(:,1) - C(i,1)).^2;
//        for j = 2:p
//            D(:,i) = D(:,i) + (X(:,j) - C(i,j)).^2;
//        end
//        % D(:,i) = sum((X - C(repmat(i,n,1),:)).^2, 2);
//    end
//    }
    /*else*/ if (m_sDistance.compare("cityblock") == 0)
    {
        for(qint32 i = 0; i < nclusts; ++i)
        {
            D.col(i) = (X.col(0).array() - C(i,0)).array().abs();
            for(qint32 j = 1; j < p; ++j)
            {
                D.col(i).array() += (X.col(j).array() - C(i,j)).array().abs();
            }
            // D(:,i) = sum(abs(X - C(repmat(i,n,1),:)), 2);
        }
//        std::cout << "D" << std::endl << D << std::endl;
    }
//    else if (m_sDistance.compare("cosine") == 0 || dist.compare("correlation") == 0)
//    {
//    % The points are normalized, centroids are not, so normalize them
//    normC = sqrt(sum(C.^2, 2));
//    if any(normC < eps(class(normC))) % small relative to unit-length data points
//        error('Zero cluster centroid created at iteration %d.',iter);
//    for i = 1:nclusts
//        D(:,i) = max(1 - X * (C(i,:)./normC(i))', 0);
//    end
//    }
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


//*************************************************************************************************************
//GCENTROIDS Centroids and counts stratified by group.
void KMeans::gcentroids(MatrixXd& X, VectorXi& index, VectorXi& clusts,
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
//        members = (index == clusts(i));
        if (c > 0)
        {
            counts[i] = c;
//            if(m_sDistance.compare("sqeuclidean") == 0)
//            {
//                centroids(i,:) = sum(X(members,:),1) / counts(i);
//            }
            /*else*/ if(m_sDistance.compare("cityblock") == 0)
            {
                // Separate out sorted coords for points in i'th cluster,
                // and use to compute a fast median, component-wise
                MatrixXd Xsorted(counts[i],p);
                c = 0;

//                std::cout << "X" << std::endl << X << std::endl;
//                qDebug() << "X " << X.rows() << "x" << X.cols();
//                qDebug() << "Xsorted " << Xsorted.rows() << "x" << Xsorted.cols();

//                std::cout << "clusts" << std::endl << clusts << std::endl;
//                std::cout << "counts " << std::endl << counts << std::endl;
//                std::cout << "m " << std::endl << m << std::endl;
//                std::cout << "index " << std::endl << index << std::endl;

                for(qint32 j = 0; j < index.rows(); ++j)
                {
                    if(index[j] == clusts[i])
                    {
//                        std::cout << "c: " << c << " j: "<< j << std::endl;
                        Xsorted.row(c) = X.row(j);
                        ++c;
                    }
                }

                for(qint32 j = 0; j < Xsorted.cols(); ++j)
                    std::sort(Xsorted.col(j).data(),Xsorted.col(j).data()+Xsorted.rows());

                //ToDo Some Bug here

//                std::cout << "Xsorted" << std::endl << Xsorted << std::endl;

                qint32 nn = floor(0.5*(counts(i)))-1;
                if (counts[i] % 2 == 0)
                    centroids.row(i) = .5 * (Xsorted.row(nn) + Xsorted.row(nn+1));
                else
                    centroids.row(i) = Xsorted.row(nn+1);
            }
//            else if(m_sDistance.compare("cosine") == 0 || dist.compare("correlation") == 0)
//            {
//                centroids(i,:) = sum(X(members,:),1) / counts(i); // unnormalized
//            }
//            else if(m_sDistance.compare("hamming") == 0)
//            {
//                % Compute a fast median for binary data, component-wise
//                centroids(i,:) = .5*sign(2*sum(X(members,:), 1) - counts(i)) + .5;
//            }


//            std::cout << "centroids" << std::endl << centroids << std::endl;
        }
    }
}// function
