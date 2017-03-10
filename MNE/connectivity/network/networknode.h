//=============================================================================================================
/**
* @file     networknode.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Lorenz Esch and Matti Hamalainen. All rights reserved.
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

#ifndef CONNECTIVITYLIB_NETWORKNODE_H
#define CONNECTIVITYLIB_NETWORKNODE_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"
#include "networkedge.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QObject>
#include <QSharedPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE NAMESPACE CONNECTIVITYLIB
//=============================================================================================================

namespace CONNECTIVITYLIB {


//*************************************************************************************************************
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

class CONNECTIVITYSHARED_EXPORT NetworkNode : public QObject
{
    Q_OBJECT

public:
    typedef QSharedPointer<NetworkNode> SPtr;            /**< Shared pointer type for NetworkNode. */
    typedef QSharedPointer<const NetworkNode> ConstSPtr; /**< Const shared pointer type for NetworkNode. */

    //=========================================================================================================
    /**
    * Constructs a NetworkNode object.
    */
    explicit NetworkNode(qint16 iId, const Eigen::RowVectorXf& vecVert, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Returns all edges(undirected gaph).
    *
    * @return   Returns the list with all edges beloning to the node.
    */
    const QList<QSharedPointer<NetworkEdge> >& getEdges() const;

    //=========================================================================================================
    /**
    * Returns the ingoing edges.
    *
    * @return   Returns the list with all ingoing edges.
    */
    const QList<QSharedPointer<NetworkEdge> >& getEdgesIn() const;

    //=========================================================================================================
    /**
    * Returns the outgoing edges.
    *
    * @return   Returns the list with all outgoing edges.
    */
    const QList<QSharedPointer<NetworkEdge>>& getEdgesOut() const;

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
    * Returns node degree.
    *
    * @return   The node degree calculated as the number of edges connected to a node (undirected gaph).
    */
    qint16 getDegree() const;

    //=========================================================================================================
    /**
    * Returns node indegree.
    *
    * @return   The node degree calculated as the number of incoming edges (only in directed graphs).
    */
    qint16 getIndegree() const;

    //=========================================================================================================
    /**
    * Returns node outdegree.
    *
    * @return   The node degree calculated as the number of outgoing edges (only in directed graphs).
    */
    qint16 getOutdegree() const;

    //=========================================================================================================
    /**
    * Returns node strength.
    *
    * @return   The node strength calculated as the sum of all weights of all edges of a node.
    */
    double getStrength() const;

    //=========================================================================================================
    /**
    * Returns node strength of all ingoing edges.
    *
    * @return   The node strength calculated as the sum of all weights of all ingoing edges of a node.
    */
    double getInstrength() const;

    //=========================================================================================================
    /**
    * Returns node strength of all outgoing edges.
    *
    * @return   The node strength calculated as the sum of all weights of all outgoing edges of a node.
    */
    double getOutstrength() const;

    //=========================================================================================================
    /**
    * Sets the hub status of this node.
    *
    * @param bIsHub   New hub status for this node.
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
    * Overloaded stream operator to add a network edge to this network node.
    *
    * @param[in] newEdge    The new edge item as a reference.
    */
    NetworkNode &operator<<(QSharedPointer<NetworkEdge> newEdge);

protected:

private:
    bool                                    m_bIsHub;

    qint16                                  m_iId;

    Eigen::RowVectorXf                      m_vecVert;

    QList<QSharedPointer<NetworkEdge> >     m_lEdgesIn;
    QList<QSharedPointer<NetworkEdge> >     m_lEdgesOut;

signals:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITYLIB_NETWORKNODE_H
