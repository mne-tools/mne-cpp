//=============================================================================================================
/**
 * @file     networknode.h
 * @author   Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Daniel Strohmeier, Lorenz Esch. All rights reserved.
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
 * @brief     NetworkNode class declaration.
 *
 */

#ifndef NETWORKNODE_H
#define NETWORKNODE_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"

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
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {

//=============================================================================================================
// CONNECTIVITYLIB FORWARD DECLARATIONS
//=============================================================================================================

class NetworkEdge;

//=============================================================================================================
/**
 * This class holds an object to describe the node of a network.
 *
 * @brief This class holds an object to describe the node of a network.
 */

class CONNECTIVITYSHARED_EXPORT NetworkNode
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
} // namespace CONNECTIVITYLIB

#endif // NETWORKNODE_H
