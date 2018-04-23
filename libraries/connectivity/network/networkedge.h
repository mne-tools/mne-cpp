//=============================================================================================================
/**
* @file     networkedge.h
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     August, 2016
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
* @brief     NetworkEdge class declaration.
*
*/

#ifndef NETWORKEDGE_H
#define NETWORKEDGE_H


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

#include<Eigen/Core>


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

class NetworkNode;


//=============================================================================================================
/**
* This class holds an object to describe the edge of a network.
*
* @brief This class holds an object to describe the edge of a network.
*/

class CONNECTIVITYSHARED_EXPORT NetworkEdge
{

public:
    typedef QSharedPointer<NetworkEdge> SPtr;            /**< Shared pointer type for NetworkEdge. */
    typedef QSharedPointer<const NetworkEdge> ConstSPtr; /**< Const shared pointer type for NetworkEdge. */

    //=========================================================================================================
    /**
    * Constructs a NetworkEdge object.
    *
    * @param[in]  pStartNode        The start node of the edge.
    * @param[in]  pEndNode          The end node of the edge.
    * @param[in]  matWeight         The edge weight.
    */
    explicit NetworkEdge(QSharedPointer<NetworkNode> pStartNode,
                         QSharedPointer<NetworkNode> pEndNode,
                         Eigen::MatrixXd& matWeight);

    //=========================================================================================================
    /**
    * Returns the start node of this edge.
    *
    * @return The start node of the edge.
    */
    QSharedPointer<NetworkNode> getStartNode();

    //=========================================================================================================
    /**
    * Returns the end node of this edge.
    *
    * @return The end node of the edge.
    */
    QSharedPointer<NetworkNode> getEndNode();

    //=========================================================================================================
    /**
    * Returns the edge weight.
    */
    Eigen::MatrixXd getWeight() const;

protected:
    QSharedPointer<NetworkNode>     m_pStartNode;       /**< The start node of the edge.*/
    QSharedPointer<NetworkNode>     m_pEndNode;         /**< The end node of the edge.*/

    Eigen::MatrixXd                 m_matWeight;        /**< The weight matrix of the edge. E.g. rows could be different frequency bins/bands and columns could be different instances in time.*/

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================



} // namespace CONNECTIVITYLIB

#endif // NETWORKEDGE_H
