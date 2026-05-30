//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networknode.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
 *           Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Implementation of @ref CONNECTIVITYLIB::NetworkNode - 3D-positioned graph node with in/out and full/thresholded incident-edge bookkeeping.
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

