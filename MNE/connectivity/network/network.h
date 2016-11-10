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

#ifndef CONNECTIVITYLIB_NETWORK_H
#define CONNECTIVITYLIB_NETWORK_H


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../connectivity_global.h"
#include "networknode.h"
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


//=============================================================================================================
/**
* Description of what this class is intended to do (in detail).
*
* @brief Brief description of this class.
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
    Eigen::MatrixXd getConnectivityMatrix();

    //=========================================================================================================
    /**
    * Returns the edges.
    */
    QList<NetworkEdge::SPtr> getEdges();

    //=========================================================================================================
    /**
    * Returns the nodes.
    */
    QList<NetworkNode::SPtr> getNodes();

    //=========================================================================================================
    /**
    * Returns network distribution, also known as network degree.
    *
    * @return   The network distribution calculated as degrees of all nodes together.
    */
    qint16 getDistribution();

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
    QString getConnectivityMethod();

    //=========================================================================================================
    /**
    * Overloaded stream operator to add a network edge to this network.
    *
    * @param[in] newEdge    The new edge item as a reference.
    */
    Network &operator<<(NetworkEdge::SPtr newEdge);

    //=========================================================================================================
    /**
    * Overloaded stream operator to add a network node to this network.
    *
    * @param[in] newNode    The new node item as a reference.
    */
    Network &operator<<(NetworkNode::SPtr newNode);

protected:

private:
    QList<NetworkEdge::SPtr>    m_lEdges;                   /**< List with all edges of the network.*/
    QList<NetworkNode::SPtr>    m_lNodes;                   /**< List with all nodes of the network.*/

    QString                     m_sConnectivityMethod;      /**< The connectivity measure method used to create the data of this network structure.*/

    //=========================================================================================================
    /**
    * Returns the connectivity matrix for this network structure.
    *
    * @return    The connectivity matrix generated from the current network information.
    */
    Eigen::MatrixXd generateConnectMat();

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITYLIB_NETWORK_H
