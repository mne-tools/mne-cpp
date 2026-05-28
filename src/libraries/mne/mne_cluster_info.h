//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2013-2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 *   Juan GPC <jgarciaprieto@mgh.harvard.edu>
 *   Gabriel Motta <gabrielbenmotta@gmail.com>
 *
 * @file mne_cluster_info.h
 * @since March 2013
 * @brief Bookkeeping for a label-restricted clustering of source-space leadfield columns.
 *
 * @ref MNELIB::MNEClusterInfo stores, for every cluster produced by
 * @ref MNEForwardSolution::cluster_forward_solution, the source-space
 * vertices it contains, the representative dipole position and the
 * mean orientation. Used to compress the gain matrix into a much
 * smaller per-cluster leadfield and to expand the clustered solution
 * back onto the cortex for visualisation.
 */

#ifndef MNE_CLUSTER_INFO_H
#define MNE_CLUSTER_INFO_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>

//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//=============================================================================================================
/**
 * Per-cluster bookkeeping for a label-restricted clustering of a forward
 * solution: contained vertices, representative dipole and mean orientation.
 *
 * @brief Cluster table used to compress and reconstruct a clustered leadfield.
 */
class MNESHARED_EXPORT MNEClusterInfo
{
public:
    typedef QSharedPointer<MNEClusterInfo> SPtr;            /**< Shared pointer type for MNEClusterInfo. */
    typedef QSharedPointer<const MNEClusterInfo> ConstSPtr; /**< Const shared pointer type for MNEClusterInfo. */

    //=========================================================================================================
    /**
     * Default constructor.
     */
    MNEClusterInfo();

    //=========================================================================================================
    /**
     * Initializes the cluster information.
     */
    void clear();

    //=========================================================================================================
    /**
     * Returns true if MNE cluster information contains no data.
     *
     * @return true if MNE cluster information is empty.
     */
    inline bool isEmpty() const;

    //=========================================================================================================
    /**
     * Returns the number of clusters
     *
     * @return number of clusters.
     */
    inline qint32 numClust() const;

    //=========================================================================================================
    /**
     * Writes the cluster info to a file
     *
     * @param[in] p_sFileName    FileName to write to.
     */
    void write(QString p_sFileName) const;

    /**
     * Overloaded == operator to compare an object to this instance.
     *
     * @param[in] object    The object which should be compared to.
     *
     * @return true if equal, false otherwise.
     */
    friend bool operator== (const MNEClusterInfo &a, const MNEClusterInfo &b);

public:
    QList<QString>              clusterLabelNames;  /**< FsLabel name of the cluster. Entries can be non unique, since some FsLabel consist of more than one cluster.*/
    QList<qint32>               clusterLabelIds;    /**< Id (FsLabel/ROI id) of the cluster. Entries can be non unique, since some FsLabel/ROI consist of more than one cluster.*/
    QList<qint32>               centroidVertno;     /**< Id (FsLabel/ROI id) of the centroid. */
    QList<Eigen::Vector3f>     centroidSource_rr;   /**< Centroid location. */
    QList<Eigen::VectorXi>     clusterVertnos;      /**< Vertnos which belong to corresponding cluster. */
    QList<Eigen::MatrixX3f>    clusterSource_rr;    /**< Cluster source locations. */
    QList<Eigen::VectorXd>     clusterDistances;    /**< Distances to clusters centroid. */
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEClusterInfo::isEmpty() const
{
    return !(this->clusterVertnos.size() > 0);
}

//=============================================================================================================

inline qint32 MNEClusterInfo::numClust() const
{
    return this->clusterVertnos.size();
}

//=============================================================================================================

inline bool operator== (const MNEClusterInfo &a, const MNEClusterInfo &b)
{
    if(a.centroidSource_rr.size() == b.centroidSource_rr.size()) {
        for(int i = 0; i < a.centroidSource_rr.size(); ++i) {
            if(!a.centroidSource_rr.at(i).isApprox(b.centroidSource_rr.at(i), 0.0001f)) {
                return false;
            }
        }
    } else {
        return false;
    }

    if(a.clusterVertnos.size() == b.clusterVertnos.size()) {
        for(int i = 0; i < a.clusterVertnos.size(); ++i) {
            if(!a.clusterVertnos.at(i).isApprox(b.clusterVertnos.at(i))) {
                return false;
            }
        }
    } else {
        return false;
    }

    if(a.clusterSource_rr.size() == b.clusterSource_rr.size()) {
        for(int i = 0; i < a.clusterSource_rr.size(); ++i) {
            if(!a.clusterSource_rr.at(i).isApprox(b.clusterSource_rr.at(i), 0.0001f)) {
                return false;
            }
        }
    } else {
        return false;
    }

    if(a.clusterDistances.size() == b.clusterDistances.size()) {
        for(int i = 0; i < a.clusterDistances.size(); ++i) {
            if(!a.clusterDistances.at(i).isApprox(b.clusterDistances.at(i))) {
                return false;
            }
        }
    } else {
        return false;
    }

    return (a.clusterLabelNames == b.clusterLabelNames &&
            a.clusterLabelIds == b.clusterLabelIds &&
            a.centroidVertno == b.centroidVertno);
}
} // NAMESPACE

#endif // MNE_CLUSTER_INFO_H
