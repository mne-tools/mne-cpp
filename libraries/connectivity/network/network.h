//=============================================================================================================
/**
* @file     network.h
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
* @brief     Network class declaration.
*
*/

#ifndef NETWORK_H
#define NETWORK_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

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
class NetworkNode;


//=============================================================================================================
/**
* This class holds information (nodes and connecting edges) about a network, can compute a distance table and provide network metrics.
*
* @brief This class holds information about a network, can compute a distance table and provide network metrics.
*/

class CONNECTIVITYSHARED_EXPORT Network
{

public:
    typedef QSharedPointer<Network> SPtr;            /**< Shared pointer type for Network. */
    typedef QSharedPointer<const Network> ConstSPtr; /**< Const shared pointer type for Network. */

    //=========================================================================================================
    /**
    * Constructs a Network object.
    *
    * @param[in] sConnectivityMethod    The connectivity measure method used to create the data of this network structure.
    */
    explicit Network(const QString& sConnectivityMethod = "Unknown");

    //=========================================================================================================
    /**
    * Returns the connectivity matrix for this network structure.
    *
    * @return    The connectivity matrix generated from the current network information.
    */
    Eigen::MatrixXd getConnectivityMatrix() const;

    //=========================================================================================================
    /**
    * Returns the edges.
    *
    * @return Returns the network edges.
    */
    const QList<QSharedPointer<NetworkEdge> >& getEdges() const;

    //=========================================================================================================
    /**
    * Returns the nodes.
    *
    * @return Returns the network nodes.
    */
    const QList<QSharedPointer<NetworkNode> >& getNodes() const;

    //=========================================================================================================
    /**
    * Returns the edge at a specific position.
    *
    * @param[in] i      The index to look up the edge. i must be a valid index position in the network list (i.e., 0 <= i < size()).
    *
    * @return Returns the network edge.
    */
    QSharedPointer<NetworkEdge> getEdgeAt(int i);

    //=========================================================================================================
    /**
    * Returns the node at a specific position.
    *
    * @param[in] i      The index to look up the node. i must be a valid index position in the network list (i.e., 0 <= i < size()).
    *
    * @return Returns the network node.
    */
    QSharedPointer<NetworkNode> getNodeAt(int i);

    //=========================================================================================================
    /**
    * Returns network distribution, also known as network degree.
    *
    * @return   The network distribution calculated as degrees of all nodes together.
    */
    qint16 getDistribution() const;

    //=========================================================================================================
    /**
    * Sets the connectivity measure method used to create the data of this network structure.
    *
    * @param[in] sConnectivityMethod    The connectivity measure method used to create the data of this network structure.
    */
    void setConnectivityMethod(const QString& sConnectivityMethod);

    //=========================================================================================================
    /**
    * Returns the connectivity measure method used to create the data of this network structure.
    *
    * @return   The connectivity measure method used to create the data of this network structure.
    */
    QString getConnectivityMethod() const;

    //=========================================================================================================
    /**
    * Appends a network edge to this network node.
    *
    * @param[in] newEdge    The new edge item.
    */
    void append(QSharedPointer<NetworkEdge> newEdge);

    //=========================================================================================================
    /**
    * Appends a network edge to this network node.
    *
    * @param[in] newNode    The new node item as a reference.
    */
    void append(QSharedPointer<NetworkNode> newNode);

protected:
    QList<QSharedPointer<NetworkEdge> >     m_lEdges;                   /**< List with all edges of the network.*/
    QList<QSharedPointer<NetworkNode> >     m_lNodes;                   /**< List with all nodes of the network.*/

    Eigen::MatrixXd                         m_matDistMatrix;            /**< The distance matrix.*/

    QString                                 m_sConnectivityMethod;      /**< The connectivity measure method used to create the data of this network structure.*/

    //=========================================================================================================
    /**
    * Returns the connectivity matrix for this network structure.
    *
    * @return    The connectivity matrix generated from the current network information.
    */
    Eigen::MatrixXd generateConnectMat() const;

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#ifndef metatype_networks
#define metatype_networks
Q_DECLARE_METATYPE(CONNECTIVITYLIB::Network);
#endif

#endif // NETWORK_H
