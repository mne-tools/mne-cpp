//=============================================================================================================
/**
* @file     network.cpp
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
* @brief    Network class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "network.h"

#include "networkedge.h"
#include "networknode.h"
#include <utils/ioutils.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Network::Network(const QString& sConnectivityMethod,
                 double dThreshold)
: m_sConnectivityMethod(sConnectivityMethod)
, m_minMaxWeightsAllEdges(QPair<double,double>(100000000000.0,0.0))
, m_minMaxWeightsActiveEdges(QPair<double,double>(100000000000.0,0.0))
, m_dThreshold(dThreshold)
{
    qRegisterMetaType<CONNECTIVITYLIB::Network>("CONNECTIVITYLIB::Network");
}


//*************************************************************************************************************

MatrixXd Network::getConnectivityMatrix() const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lEdges.size(); ++i) {
        int row = m_lEdges.at(i)->getStartNodeID();
        int col = m_lEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols()) {
            matDist(row,col) = m_lEdges.at(i)->getWeight();
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}


//*************************************************************************************************************

const QList<NetworkEdge::SPtr>& Network::getEdges() const
{
    return m_lActiveEdges;
}


//*************************************************************************************************************

const QList<NetworkNode::SPtr>& Network::getNodes() const
{
    return m_lNodes;
}


//*************************************************************************************************************

NetworkEdge::SPtr Network::getEdgeAt(int i)
{
    return m_lActiveEdges.at(i);
}


//*************************************************************************************************************

NetworkNode::SPtr Network::getNodeAt(int i)
{
    return m_lNodes.at(i);
}


//*************************************************************************************************************

qint16 Network::getDistribution() const
{
    qint16 distribution = 0;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        distribution += m_lNodes.at(i)->getDegree();
    }

    return distribution;
}


//*************************************************************************************************************

void Network::setConnectivityMethod(const QString& sConnectivityMethod)
{
    m_sConnectivityMethod = sConnectivityMethod;
}


//*************************************************************************************************************

QString Network::getConnectivityMethod() const
{
    return m_sConnectivityMethod;
}


//*************************************************************************************************************

QPair<double, double> Network::getMinMaxWeights() const
{
    return m_minMaxWeightsActiveEdges;
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxDegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getDegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getDegree();
        }

        if(m_lNodes.at(i)->getDegree() < minDegree){
            minDegree = m_lNodes.at(i)->getDegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxIndegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getIndegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getIndegree();
        }

        if(m_lNodes.at(i)->getIndegree() < minDegree){
            minDegree = m_lNodes.at(i)->getIndegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxOutdegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getOutdegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getOutdegree();
        }

        if(m_lNodes.at(i)->getOutdegree() < minDegree){
            minDegree = m_lNodes.at(i)->getOutdegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

void Network::setThreshold(double dThreshold)
{
    m_dThreshold = dThreshold;
    m_lActiveEdges.clear();

    for(int i = 0; i < m_lEdges.size(); ++i) {
        if(m_lEdges.at(i)->getWeight() >= m_dThreshold) {
            m_lEdges.at(i)->setActive(true);
            m_lActiveEdges.append(m_lEdges.at(i));
        } else {
            m_lEdges.at(i)->setActive(false);
        }
    }

    m_minMaxWeightsActiveEdges.first = m_dThreshold;
    m_minMaxWeightsActiveEdges.second = m_minMaxWeightsAllEdges.second;
}


//*************************************************************************************************************

void Network::append(NetworkEdge::SPtr newEdge)
{
    if(newEdge->getWeight() < m_minMaxWeightsAllEdges.first) {
        m_minMaxWeightsAllEdges.first = newEdge->getWeight();
    } else if(newEdge->getWeight() >= m_minMaxWeightsAllEdges.second) {
        m_minMaxWeightsAllEdges.second = newEdge->getWeight();
    }

    m_lEdges << newEdge;
}


//*************************************************************************************************************

void Network::append(NetworkNode::SPtr newNode)
{
    m_lNodes << newNode;
}


//*************************************************************************************************************

bool Network::isEmpty() const
{
    if(m_lEdges.isEmpty() || m_lNodes.isEmpty()) {
        return true;
    }

    return false;
}

