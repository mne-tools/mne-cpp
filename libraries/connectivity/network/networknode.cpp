//=============================================================================================================
/**
 * @file     networknode.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch. All rights reserved.
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
 * @brief    NetworkNode class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networknode.h"

#include "networkedge.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkNode::NetworkNode(qint16 iId, const RowVectorXf& vecVert)
: m_bIsHub(false)
, m_iId(iId)
, m_vecVert(vecVert)
{
}

//=============================================================================================================

const QList<QSharedPointer<NetworkEdge> >& NetworkNode::getFullEdges() const
{
    return m_lEdges;
}

//=============================================================================================================

QList<QSharedPointer<NetworkEdge> > NetworkNode::getThresholdedEdges() const
{
    QList<QSharedPointer<NetworkEdge> > edgeList;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive()) {
            edgeList << m_lEdges.at(i);
        }
    }

    return edgeList;
}

//=============================================================================================================

QList<QSharedPointer<NetworkEdge> > NetworkNode::getFullEdgesIn() const
{
    QList<QSharedPointer<NetworkEdge> > edgeList;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            edgeList << m_lEdges.at(i);
        }
    }

    return edgeList;
}

//=============================================================================================================

QList<QSharedPointer<NetworkEdge> > NetworkNode::getThresholdedEdgesIn() const
{
    QList<QSharedPointer<NetworkEdge> > edgeList;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            edgeList << m_lEdges.at(i);
        }
    }

    return edgeList;
}

//=============================================================================================================

QList<QSharedPointer<NetworkEdge> > NetworkNode::getFullEdgesOut() const
{
    QList<QSharedPointer<NetworkEdge> > edgeList;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            edgeList << m_lEdges.at(i);
        }
    }

    return edgeList;
}

//=============================================================================================================

QList<QSharedPointer<NetworkEdge> > NetworkNode::getThresholdedEdgesOut() const
{
    QList<QSharedPointer<NetworkEdge> > edgeList;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            edgeList << m_lEdges.at(i);
        }
    }

    return edgeList;
}

//=============================================================================================================

const RowVectorXf& NetworkNode::getVert() const
{
    return m_vecVert;
}

//=============================================================================================================

qint16 NetworkNode::getId() const
{
    return m_iId;
}

//=============================================================================================================

qint16 NetworkNode::getFullDegree() const
{
    return m_lEdges.size();
}

//=============================================================================================================

qint16 NetworkNode::getThresholdedDegree() const
{
    qint16 degree = 0;

    for(int i = 0; i < m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive()) {
            degree++;
        }
    }

    return degree;
}

//=============================================================================================================

qint16 NetworkNode::getFullIndegree() const
{
    qint16 degree = 0;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            degree++;
        }
    }

    return degree;
}

//=============================================================================================================

qint16 NetworkNode::getThresholdedIndegree() const
{
    qint16 degree = 0;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            degree++;
        }
    }

    return degree;
}

//=============================================================================================================

qint16 NetworkNode::getFullOutdegree() const
{
    qint16 degree = 0;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            degree++;
        }
    }

    return degree;
}

//=============================================================================================================

qint16 NetworkNode::getThresholdedOutdegree() const
{
    qint16 degree = 0;

    for(int i = 0; i< m_lEdges.size(); i++) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            degree++;
        }
    }

    return degree;
}

//=============================================================================================================

double NetworkNode::getFullStrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        dStrength += m_lEdges.at(i)->getWeight();
    }

    return dStrength;
}

//=============================================================================================================

double NetworkNode::getThresholdedStrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->isActive()) {
            dStrength += m_lEdges.at(i)->getWeight();
        }
    }

    return dStrength;
}

//=============================================================================================================

double NetworkNode::getFullInstrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            dStrength += m_lEdges.at(i)->getWeight();
        }
    }

    return dStrength;
}

//=============================================================================================================

double NetworkNode::getThresholdedInstrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getEndNodeID() == this->getId()) {
            dStrength += m_lEdges.at(i)->getWeight();
        }
    }

    return dStrength;
}

//=============================================================================================================

double NetworkNode::getFullOutstrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            dStrength += m_lEdges.at(i)->getWeight();
        }
    }

    return dStrength;
}

//=============================================================================================================

double NetworkNode::getThresholdedOutstrength() const
{
    double dStrength = 0.0;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->isActive() && m_lEdges.at(i)->getStartNodeID() == this->getId()) {
            dStrength += m_lEdges.at(i)->getWeight();
        }
    }

    return dStrength;
}

//=============================================================================================================

void NetworkNode::setHubStatus(bool bIsHub)
{
    m_bIsHub = bIsHub;
}

//=============================================================================================================

bool NetworkNode::getHubStatus() const
{
    return m_bIsHub;
}

//=============================================================================================================

void NetworkNode::append(QSharedPointer<NetworkEdge> newEdge)
{
    if(newEdge->getEndNodeID() != newEdge->getStartNodeID()) {
        m_lEdges << newEdge;
    }
}

