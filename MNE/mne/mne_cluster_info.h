//=============================================================================================================
/**
* @file     mne_cluster_info.h
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
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
* @brief    MNEClusterInfo class declaration, which provides cluster information.
*
*/

#ifndef MNE_CLUSTER_INFO_H
#define MNE_CLUSTER_INFO_H

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "mne_global.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSharedPointer>
#include <QList>


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE MNELIB
//=============================================================================================================

namespace MNELIB
{

//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;


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
    * @param[in] p_sFileName    FileName to write to
    */
    void write(QString p_sFileName) const;

public:
    QList<QString>      clusterLabelNames;  /**< Label name of the cluster. Entries can be non unique, since some Label consist of more than one cluster.*/
    QList<qint32>       clusterLabelIds;    /**< Id (Label/ROI id) of the cluster. Entries can be non unique, since some Label/ROI consist of more than one cluster.*/
    QList<qint32>       centroidVertno;     /**< Id (Label/ROI id) of the centroid */
    QList<Vector3f>     centroidSource_rr;  /**< Centroid location */
    QList<VectorXi>     clusterVertnos;     /**< Vertnos which belong to corresponding cluster. */
    QList<MatrixX3f>    clusterSource_rr;   /**< Cluster source locations */
    QList<VectorXd>     clusterDistances;   /**< Distances to clusters centroid. */
};

//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================

inline bool MNEClusterInfo::isEmpty() const
{
    return !(this->clusterVertnos.size() > 0);
}


//*************************************************************************************************************

inline qint32 MNEClusterInfo::numClust() const
{
    return this->clusterVertnos.size();
}

} // NAMESPACE

#endif // MNE_CLUSTER_INFO_H
