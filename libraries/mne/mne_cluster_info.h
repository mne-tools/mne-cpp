//=============================================================================================================
/**
 * @file     mne_cluster_info.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2012
 *
 * @section  LICENSE
 *
 * Copyright (C) 2012, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
 * @brief    MNEClusterInfo class declaration, which provides cluster information.
 *
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
 * Cluster information
 *
 * @brief cluster information
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
    QList<QString>              clusterLabelNames;  /**< Label name of the cluster. Entries can be non unique, since some Label consist of more than one cluster.*/
    QList<qint32>               clusterLabelIds;    /**< Id (Label/ROI id) of the cluster. Entries can be non unique, since some Label/ROI consist of more than one cluster.*/
    QList<qint32>               centroidVertno;     /**< Id (Label/ROI id) of the centroid. */
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
