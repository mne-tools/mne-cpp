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

#include "connectivity_global.h"

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
    explicit NetworkNode(qint16 iNodeNumber, const Eigen::RowVectorXf& vecVert, QObject *parent = 0);

    //=========================================================================================================
    /**
    * Returns the in going edges.
    */
    QList<NetworkEdge::SPtr> getEdgesIn();

    //=========================================================================================================
    /**
    * Returns the in outgoing edges.
    */
    QList<NetworkEdge::SPtr> getEdgesOut();

    //=========================================================================================================
    /**
    * Returns the node number.
    */
    qint16 getNodeNumber();

    //=========================================================================================================
    /**
    * Overloaded stream operator to add a network edge to this network node.
    *
    * @param[in] newEdge    The new edge item as a reference.
    */
    NetworkNode &operator<<(NetworkEdge::SPtr newEdge);

protected:

private:
    qint16                      m_iNodeNumber;

    Eigen::RowVectorXf          m_vecVert;

    QList<NetworkEdge::SPtr>    m_lEdgesIn;
    QList<NetworkEdge::SPtr>    m_lEdgesOut;

signals:

};


//*************************************************************************************************************
//=============================================================================================================
// INLINE DEFINITIONS
//=============================================================================================================


} // namespace CONNECTIVITYLIB

#endif // CONNECTIVITYLIB_NETWORKNODE_H
