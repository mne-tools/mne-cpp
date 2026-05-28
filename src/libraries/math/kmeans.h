//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *
 * @file     kmeans.h
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    2.0.0
 * @date     March 2026
 * @brief    K-means partitional clustering with multiple distance metrics, initialisations and empty-cluster policies.
 *
 * Lloyd-style K-means partitions @c n p-dimensional samples into @c k
 * clusters by alternating an assignment step (each point goes to the
 * nearest centroid under the selected metric) with an update step
 * (centroid ← mean of its members) until the total within-cluster
 * distortion stops decreasing or the iteration cap is hit. Per-iteration
 * cost is @c O(n*k*p), which dominates everything else in MATHLIB and
 * makes the algorithm suitable for the cortical-patch decimation and
 * dipole-cluster prior workflows that consume it inside INVERSELIB.
 *
 * The implementation mirrors MATLAB's @c kmeans/@c kmeans2 in feature
 * set: five distance metrics (squared Euclidean, city-block, cosine,
 * correlation, Hamming) selectable via @ref KMeansDistance, three
 * seeding strategies (random sample, uniform within data range,
 * subsample-then-cluster) via @ref KMeansStart, multiple replicates
 * with best-of selection, and three empty-cluster policies (error,
 * drop, singleton-from-farthest-point) via @ref KMeansEmptyAction. An
 * optional online update phase performs single-point Lloyd moves after
 * the batch loop converges and is enabled by default to escape shallow
 * local minima.
 *
 * Reference: Lloyd (1982) "Least squares quantization in PCM";
 * Arthur & Vassilvitskii (2007) for the seeding intuition.
 */

#ifndef KMEANS_H
#define KMEANS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "math_global.h"

//=============================================================================================================
// STL INCLUDES
//=============================================================================================================

#include <random>
#include <string>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// DEFINE NAMESPACE UTILSLIB
//=============================================================================================================

namespace UTILSLIB
{

//=============================================================================================================
// ENUMS
//=============================================================================================================

/** @brief Distance metric for K-Means clustering. */
enum class KMeansDistance
{
    SquaredEuclidean,   /**< Squared Euclidean distance (default). */
    CityBlock,          /**< Manhattan / city-block distance. */
    Cosine,             /**< Cosine distance. */
    Correlation,        /**< Correlation distance. */
    Hamming             /**< Hamming distance (binary data only). */
};

/** @brief Initialization strategy for K-Means clustering. */
enum class KMeansStart
{
    Sample,             /**< Random sample of data points (default). */
    Uniform,            /**< Uniform random within data range. */
    Cluster             /**< Sub-sample then cluster. */
};

/** @brief Action to take when a K-Means cluster becomes empty. */
enum class KMeansEmptyAction
{
    Error,              /**< Treat as an error (default). */
    Drop,               /**< Drop the empty cluster. */
    Singleton           /**< Replace with the farthest point from its centroid. */
};

//=============================================================================================================
/**
 * Lloyd-style K-means partitional clustering with configurable distance
 * metric, seeding strategy, replicate count, empty-cluster policy and
 * optional online refinement phase.
 *
 * @brief Lloyd-style K-means clustering with configurable metric, seeding and replicates.
 */
class MATHSHARED_EXPORT KMeans
{
public:
    typedef QSharedPointer<KMeans> SPtr;            /**< Shared pointer type for KMeans. */
    typedef QSharedPointer<const KMeans> ConstSPtr; /**< Const shared pointer type for KMeans. */

    //=========================================================================================================
    /**
     * Constructs a KMeans algorithm object.
     *
     * @param[in] distance   (optional) K-Means distance measure: "sqeuclidean" (default), "cityblock" , "cosine", "correlation", "hamming".
     * @param[in] start      (optional) Cluster initialization: "sample" (default), "uniform", "cluster".
     * @param[in] replicates (optional) Number of K-Means replicates, which are generated. Best is returned.
     * @param[in] emptyact   (optional) What happens if a cluster goes empty: "error" (default), "drop", "singleton".
     * @param[in] online     (optional) If centroids should be updated during iterations: true (default), false.
     * @param[in] maxit      (optional) Maximal number of iterations per replicate; 100 by default.
     */
    explicit KMeans(QString distance = QString("sqeuclidean"),
                    QString start = QString("sample"),
                    qint32 replicates = 1,
                    QString emptyact = QString("error"),
                    bool online = true,
                    qint32 maxit = 100);

    //=========================================================================================================
    /**
     * Constructs a KMeans algorithm object using enum parameters (preferred for performance).
     *
     * @param[in] distance   Distance metric.
     * @param[in] start      Cluster initialization strategy.
     * @param[in] replicates Number of K-Means replicates. Best is returned.
     * @param[in] emptyact   What happens if a cluster goes empty.
     * @param[in] online     If centroids should be updated during iterations.
     * @param[in] maxit      Maximal number of iterations per replicate.
     */
    explicit KMeans(KMeansDistance distance,
                    KMeansStart start = KMeansStart::Sample,
                    qint32 replicates = 1,
                    KMeansEmptyAction emptyact = KMeansEmptyAction::Error,
                    bool online = true,
                    qint32 maxit = 100);

    //=========================================================================================================
    /**
     * Clusters input data X.
     *
     * @param[in] X          Input data (rows = points; cols = p dimensional space).
     * @param[in] kClusters  Number of k clusters.
     * @param[out] idx       The cluster indices to which cluster the input points belong to.
     * @param[out] C         Cluster centroids k x p.
     * @param[out] sumD      Summation of the distances to the centroid within one cluster.
     * @param[out] D         Cluster distances to the centroid.
     *
     * @return true if clustering succeeded, false otherwise.
     */
    bool calculate(const Eigen::MatrixXd& X,
                   qint32 kClusters,
                   Eigen::VectorXi& idx,
                   Eigen::MatrixXd& C,
                   Eigen::VectorXd& sumD,
                   Eigen::MatrixXd& D);

private:
    //=========================================================================================================
    /**
     * Calculate point-to-cluster-centroid distances.
     *
     * @param[in] X  Input data (rows = points; cols = p dimensional space).
     * @param[in] C  Cluster centroids.
     *
     * @return n x k distance matrix.
     */
    Eigen::MatrixXd distfun(const Eigen::MatrixXd& X,
                            const Eigen::MatrixXd& C);

    //=========================================================================================================
    /**
     * Batch-update step: reassign all points to nearest centroid and recompute centroids.
     *
     * @param[in] X          Input data.
     * @param[in, out] C     Cluster centroids.
     * @param[in, out] idx   Cluster indices for each point.
     *
     * @return true if converged, false otherwise.
     */
    bool batchUpdate(const Eigen::MatrixXd& X,
                     Eigen::MatrixXd& C,
                     Eigen::VectorXi& idx);

    //=========================================================================================================
    /**
     * Compute centroids and point counts for the given clusters.
     *
     * @param[in] X          Input data.
     * @param[in] index      Cluster index for each point.
     * @param[in] clusts     Cluster labels to recompute.
     * @param[out] centroids Recomputed centroids.
     * @param[out] counts    Number of points per cluster.
     */
    void gcentroids(const Eigen::MatrixXd& X,
                    const Eigen::VectorXi& index,
                    const Eigen::VectorXi& clusts,
                    Eigen::MatrixXd& centroids,
                    Eigen::VectorXi& counts);

    //=========================================================================================================
    /**
     * Online-update step: single-point reassignments to refine cluster assignments.
     *
     * @param[in] X          Input data.
     * @param[in, out] C     Cluster centroids.
     * @param[in, out] idx   Cluster indices for each point.
     *
     * @return true if converged, false otherwise.
     */
    bool onlineUpdate(const Eigen::MatrixXd& X,
                      Eigen::MatrixXd& C,
                      Eigen::VectorXi& idx);

    //=========================================================================================================
    /**
     * Maps a distance string name to the corresponding enum.
     *
     * @param[in] name  Distance name ("sqeuclidean", "cityblock", etc.).
     * @return The matching KMeansDistance enum value.
     */
    static KMeansDistance distanceFromString(const std::string& name);

    //=========================================================================================================
    /**
     * Maps a start-method string name to the corresponding enum.
     *
     * @param[in] name  Start name ("sample", "uniform", "cluster").
     * @return The matching KMeansStart enum value.
     */
    static KMeansStart startFromString(const std::string& name);

    //=========================================================================================================
    /**
     * Maps an empty-action string name to the corresponding enum.
     *
     * @param[in] name  Empty-action name ("error", "drop", "singleton").
     * @return The matching KMeansEmptyAction enum value.
     */
    static KMeansEmptyAction emptyactFromString(const std::string& name);

    KMeansDistance     m_distance;    /**< Distance metric to use. */
    KMeansStart        m_start;       /**< Initialization strategy. */
    KMeansEmptyAction  m_emptyact;    /**< Empty-cluster action. */
    qint32 m_iReps;                   /**< Number of replicates. */
    qint32 m_iMaxit;                  /**< Max iterations per replicate. */
    bool   m_bOnline;                 /**< Whether to perform online updates. */

    std::mt19937 m_rng;               /**< Mersenne Twister random number generator. */

    qint32 emptyErrCnt;               /**< Empty-cluster error count. */
    qint32 iter;                      /**< Current iteration. */
    qint32 k;                         /**< Number of clusters. */
    qint32 n;                         /**< Number of data points. */
    qint32 p;                         /**< Dimensionality of the data space. */

    Eigen::MatrixXd Del;              /**< Reassignment cost matrix (n x k). */
    Eigen::VectorXd d;                /**< Minimal distance of each point to its centroid. */
    Eigen::VectorXi m;                /**< Number of points in each cluster. */

    double totsumD;                   /**< Total sum of distances for current assignment. */
    double prevtotsumD;               /**< Total sum of distances from previous iteration. */
    Eigen::VectorXi previdx;          /**< Previous cluster indices. */
};
} // NAMESPACE

#endif // KMEANS_H
