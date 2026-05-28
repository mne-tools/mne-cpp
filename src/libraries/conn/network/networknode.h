//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2026 MNE-CPP Authors
 *   Christoph Dinh <christoph.dinh@mne-cpp.org>
 *
 * @file networknode.h
 * @since 2026
 * @date  March 2026
 * @brief Node of a connectivity @ref Network; carries a 3D position and the lists of incident (in / out, full / thresholded) edges.
 *
 * One @ref NetworkNode is created per sensor (in sensor-space connectivity)
 * or per source vertex / ROI centroid (in source-space connectivity). The
 * 3D position is filled by @ref ConnectivitySettings::setNodePositions
 * from either a @c FiffInfo channel set or from a forward solution + a
 * @c FsSurfaceSet pair, and is used by disp3D to draw the node at the
 * correct anatomical location.
 *
 * The node also stores the lists of incident edges - separately for the
 * full unthresholded graph and for the thresholded view, and separately
 * for ingoing and outgoing directions - so that graph-theoretic measures
 * (degree, hub-ness, in/out strength) can be computed in O(1) per node
 * once the corresponding @ref Network has been assembled.
 */

#ifndef NETWORKNODE_H
#define NETWORKNODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../conn_global.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

//=============================================================================================================
// DEFINE NAMESPACE CONNLIB
//=============================================================================================================

namespace CONNLIB {

//=============================================================================================================
// CONNLIB FORWARD DECLARATIONS
//=============================================================================================================

class NetworkEdge;

//=============================================================================================================
/**
 * Node of a @ref Network. Stores a unique ID, a 3D anatomical position
 * (sensor coordinate or source vertex) and the lists of incident edges
 * partitioned by direction (in / out) and by threshold state (full /
 * thresholded), so that graph-theoretic measures can be read off without
 * rescanning the global edge list.
 *
 * @brief Graph node carrying a 3D position and its incident in/out, full/thresholded edge lists.
 */

class CONNSHARED_EXPORT NetworkNode
{

public:
    typedef QSharedPointer<NetworkNode> SPtr;            /**< Shared pointer type for NetworkNode. */
    typedef QSharedPointer<const NetworkNode> ConstSPtr; /**< Const shared pointer type for NetworkNode. */

    //=========================================================================================================
    /**
     * Constructs a NetworkNode object.
     *
     * @param[in] iId        The node's ID.
     * @param[in] vecVert    The node's 3D position.
     */
    explicit NetworkNode(qint16 iId, const Eigen::RowVectorXf& vecVert);

    //=========================================================================================================
    /**
     * Returns the ingoing edges corresponding to the full network.
     *
     * @return   Returns the list with all ingoing edges.
     */
    const QList<QSharedPointer<NetworkEdge> >& getFullEdges() const;

    //=========================================================================================================
    /**
     * Returns all edges corresponding to the thresholded network.
     *
     * @return   Returns the list with all ingoing edges.
     */
    QList<QSharedPointer<NetworkEdge> > getThresholdedEdges() const;

    //=========================================================================================================
    /**
     * Returns the ingoing edges corresponding to the full network.
     *
     * @return   Returns the list with all ingoing edges.
     */
    QList<QSharedPointer<NetworkEdge> > getFullEdgesIn() const;

    //=========================================================================================================
    /**
     * Returns the ingoing edges corresponding to the thresholded network.
     *
     * @return   Returns the list with all ingoing edges.
     */
    QList<QSharedPointer<NetworkEdge> > getThresholdedEdgesIn() const;

    //=========================================================================================================
    /**
     * Returns the outgoing edges corresponding to the full network.
     *
     * @return   Returns the list with all outgoing edges.
     */
    QList<QSharedPointer<NetworkEdge> > getFullEdgesOut() const;

    //=========================================================================================================
    /**
     * Returns the outgoing edges corresponding to the thresholded network.
     *
     * @return   Returns the list with all outgoing edges.
     */
    QList<QSharedPointer<NetworkEdge> > getThresholdedEdgesOut() const;

    //=========================================================================================================
    /**
     * Returns the vertex (position) of the node.
     *
     * @return   Returns the 3D position of the node.
     */
    const Eigen::RowVectorXf& getVert() const;

    //=========================================================================================================
    /**
     * Returns the node id.
     *
     * @return   Returns the node id.
     */
    qint16 getId() const;

    //=========================================================================================================
    /**
     * Returns node degree corresponding to the full network.
     *
     * @return   The node degree calculated as the number of edges connected to a node (undirected gaph).
     */
    qint16 getFullDegree() const;

    //=========================================================================================================
    /**
     * Returns node degree corresponding to the thresholded network.
     *
     * @return   The node degree calculated as the number of edges connected to a node (undirected gaph).
     */
    qint16 getThresholdedDegree() const;

    //=========================================================================================================
    /**
     * Returns node indegree corresponding to the full network.
     *
     * @return   The node degree calculated as the number of incoming edges (only in directed graphs).
     */
    qint16 getFullIndegree() const;

    //=========================================================================================================
    /**
     * Returns node indegree corresponding to the thresholded network.
     *
     * @return   The node degree calculated as the number of incoming edges (only in directed graphs).
     */
    qint16 getThresholdedIndegree() const;

    //=========================================================================================================
    /**
     * Returns node outdegree corresponding to the full network.
     *
     * @return   The node degree calculated as the number of outgoing edges (only in directed graphs).
     */
    qint16 getFullOutdegree() const;

    //=========================================================================================================
    /**
     * Returns node outdegree corresponding to the thresholded network.
     *
     * @return   The node degree calculated as the number of outgoing edges (only in directed graphs).
     */
    qint16 getThresholdedOutdegree() const;

    //=========================================================================================================
    /**
     * Returns node strength corresponding to the full network.
     *
     * @return   The node strength calculated as the sum of all weights of all edges of a node.
     */
    double getFullStrength() const;

    //=========================================================================================================
    /**
     * Returns node strength corresponding to the thresholded network.
     *
     * @return   The node strength calculated as the sum of all weights of all edges of a node.
     */
    double getThresholdedStrength() const;

    //=========================================================================================================
    /**
     * Returns node strength of all ingoing edges corresponding to the full network.
     *
     * @return   The node strength calculated as the sum of all weights of all ingoing edges of a node.
     */
    double getFullInstrength() const;

    //=========================================================================================================
    /**
     * Returns node strength of all ingoing edges corresponding to the thresholded network.
     *
     * @return   The node strength calculated as the sum of all weights of all ingoing edges of a node.
     */
    double getThresholdedInstrength() const;

    //=========================================================================================================
    /**
     * Returns node strength of all outgoing edges corresponding to the full network.
     *
     * @return   The node strength calculated as the sum of all weights of all outgoing edges of a node.
     */
    double getFullOutstrength() const;

    //=========================================================================================================
    /**
     * Returns node strength of all outgoing edges corresponding to the thresholded network.
     *
     * @return   The node strength calculated as the sum of all weights of all outgoing edges of a node.
     */
    double getThresholdedOutstrength() const;

    //=========================================================================================================
    /**
     * Sets the hub status of this node.
     *
     * @param[in] bIsHub   New hub status for this node.
     */
    void setHubStatus(bool bIsHub);

    //=========================================================================================================
    /**
     * Returns flag describing whether this node is a hub or not.
     *
     * @return   Whether this node is a hub or not.
     */
    bool getHubStatus() const;

    //=========================================================================================================
    /**
     * Appends a network edge to this network node. Automatically decides whether to add to the in or out edges.
     *
     * @param[in] newEdge    The new edge item.
     */
    void append(QSharedPointer<NetworkEdge> newEdge);

protected:
    bool                                    m_bIsHub;       /**< Whether this node is a hub.*/

    qint16                                  m_iId;          /**< The node's ID.*/

    Eigen::RowVectorXf                      m_vecVert;      /**< The 3D position of the node.*/

    QList<QSharedPointer<NetworkEdge> >     m_lEdges;     /**< List with all incoming edges of the node.*/
};

//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================
} // namespace CONNLIB

#endif // NETWORKNODE_H
