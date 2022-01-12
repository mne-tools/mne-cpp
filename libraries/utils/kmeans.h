//=============================================================================================================
/**
 * @file     kmeans.h
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
 * @brief    KMeans class declaration.
 *
 */

#ifndef KMEANS_H
#define KMEANS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "utils_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <string>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
/**
 * K-Means Clustering
 *
 * @brief K-Means Clustering
 */
class UTILSSHARED_EXPORT KMeans
{
public:
    typedef QSharedPointer<KMeans> SPtr;            /**< Shared pointer type for KMeans. */
    typedef QSharedPointer<const KMeans> ConstSPtr; /**< Const shared pointer type for KMeans. */

    //distance {'sqeuclidean','cityblock','cosine','correlation','hamming'};
    //startNames = {'uniform','sample','cluster'};
    //emptyactNames = {'error','drop','singleton'};

    //=========================================================================================================
    /**
     * Constructs a KMeans algorithm object.
     *
     * @param[in] distance   (optional) K-Means distance measure: "sqeuclidean" (default), "cityblock" , "cosine", "correlation", "hamming".
     * @param[in] start      (optional) Cluster initialization: "sample" (default), "uniform", "cluster".
     * @param[in] replicates (optional) Number of K-Means replicates, which are generated. Best is returned.
     * @param[in] emptyact   (optional) What happens if a cluster wents empty: "error" (default), "drop", "singleton".
     * @param[in] online     (optional) If centroids should be updated during iterations: true (default), false.
     * @param[in] maxit      (optional) maximal number of iterations per replicate; 100 by default.
     */
    explicit KMeans(QString distance = QString("sqeuclidean") ,
                    QString start = QString("sample"),
                    qint32 replicates = 1,
                    QString emptyact = QString("error"),
                    bool online = true,
                    qint32 maxit = 100);

//    //=========================================================================================================
//    /**
//     * Constructs a KMeans algorithm object.
//     *
//     * @param[in] distance   (optional) K-Means distance measure: "sqeuclidean" (default), "cityblock" , "cosine", "correlation", "hamming".
//     * @param[in] start      (optional) Cluster initialization: "sample" (default), "uniform", "cluster".
//     * @param[in] replicates (optional) Number of K-Means replicates, which are generated. Best is returned.
//     * @param[in] emptyact   (optional) What happens if a cluster wents empty: "error" (default), "drop", "singleton".
//     * @param[in] online     (optional) If centroids should be updated during iterations: true (default), false.
//     * @param[in] maxit      (optional) maximal number of iterations per replicate; 100 by default.
//     */
//    explicit KMeans(std::string distance = std::string{"sqeuclidean"} ,
//                    std::string start = std::string{"sample"},
//                    qint32 replicates = 1,
//                    std::string emptyact = std::string{"error"},
//                    bool online = true,
//                    qint32 maxit = 100);

    //=========================================================================================================
    /**
     * Clusters input data X
     *
     * @param[in] X          Input data (rows = points; cols = p dimensional space).
     * @param[in] kClusters  Number of k clusters.
     * @param[out] idx       The cluster indeces to which cluster the input points belong to.
     * @param[out] C         Cluster centroids k x p.
     * @param[out] sumD      Summation of the distances to the centroid within one cluster.
     * @param[out] D         Cluster distances to the centroid.
     */
    bool calculate( Eigen::MatrixXd X,
                    qint32 kClusters,
                    Eigen::VectorXi& idx,
                    Eigen::MatrixXd& C,
                    Eigen::VectorXd& sumD,
                    Eigen::MatrixXd& D);

private:
    //=========================================================================================================
    /**
     * Calculate point to cluster centroid distances.
     *
     * @param[in] X  Input data (rows = points; cols = p dimensional space).
     * @param[in] C  Cluster centroids.
     *
     * @return Cluster centroid distances.
     */
    Eigen::MatrixXd distfun(const Eigen::MatrixXd& X,
                            Eigen::MatrixXd& C);//, qint32 iter);

    //=========================================================================================================
    /**
     * Updates clusters when points moved
     *
     * @param[in] X          Input data.
     * @param[in, out] C     Cluster centroids.
     * @param[in, out] idx   The cluster indeces to which cluster the input points belong to.
     *
     * @return true if converged, false otherwise.
     */
    bool batchUpdate(const Eigen::MatrixXd& X,
                     Eigen::MatrixXd& C,
                     Eigen::VectorXi& idx);

    //=========================================================================================================
    /**
     * Centroids and counts stratified by group.
     *
     * @param[in] X          Input data.
     * @param[in] index      The cluster indeces to which cluster the input points belong to.
     * @param[in] clusts     Cluster indeces.
     * @param[out] centroids The new centroids.
     * @param[out] counts    Number of points belonging to the new centroids.
     */
    void gcentroids(const Eigen::MatrixXd& X,
                    const Eigen::VectorXi& index,
                    const Eigen::VectorXi& clusts,
                    Eigen::MatrixXd& centroids,
                    Eigen::VectorXi& counts);

    //=========================================================================================================
    /**
     * Centroids and counts stratified by group.
     *
     * @param[in] X          Input data.
     * @param[out] C         The new centroids.
     * @param[out] idx       The new indeces.
     *
     * @return true if converged, false otherwise.
     */
    bool onlineUpdate(const Eigen::MatrixXd& X,
                      Eigen::MatrixXd& C,
                      Eigen::VectorXi& idx);

    //=========================================================================================================
    /**
     * Uniform random generator in the intervall [a, b]
     *
     * @param[in] a      lower boundary.
     * @param[in] b      upper boundary.
     *
     * @return random number.
     */
    double unifrnd(double a, double b);

    std::string m_sDistance;    /**< Distance measurement to use: "sqeuclidean" (default), "cityblock" , "cosine", "correlation", "hamming". */
    std::string m_sStart;       /**< Initialization to use: "sample" (default), "uniform", "cluster". */
    qint32 m_iReps;         /**< Number of K-Means replicates, which should be generated. */
    std::string m_sEmptyact;    /**< What should be done if a cluster wents empty: "error" (default), "drop", "singleton". */
    qint32 m_iMaxit;        /**< Maximal number of iterations per replicate. */
    bool m_bOnline;         /**< If online update should be performed. */

    qint32 emptyErrCnt;     /**< Counts the occurence of empty errors. */

    qint32 iter;            /**< Current iteration. */
    qint32 k;               /**< Number of clusters. */
    qint32 n;               /**< Number of points to be clustered. */
    qint32 p;               /**< dimension of space in which the clustering is performed. */

    Eigen::MatrixXd Del;    /**< reassignment criterion. */
    Eigen::VectorXd d;      /**< Minimal distances of each point to its centroid. */
    Eigen::VectorXi m;      /**< m number of points belonging to the cluster. */

    double totsumD;         /**< Total sum of centroid distances. */

    double prevtotsumD;     /**< Sum of centroid distances of the previous iteration. */

    Eigen::VectorXi previdx;/**< Previous point cluster indeces. */
};
} // NAMESPACE

#endif // KMEANS_H
