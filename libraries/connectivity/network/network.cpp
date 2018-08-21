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

Network::Network(const QString& sConnectivityMethod)
: m_sConnectivityMethod(sConnectivityMethod)
, m_minMaxWeights(QPair<float,float>(100000000.0f,0.0f))
{
    qRegisterMetaType<CONNECTIVITYLIB::Network>("CONNECTIVITYLIB::Network");
}


//*************************************************************************************************************

MatrixXd Network::getConnectivityMatrix() const
{
    return generateConnectMat();
}


//*************************************************************************************************************

const QList<NetworkEdge::SPtr>& Network::getEdges() const
{
    return m_lEdges;
}


//*************************************************************************************************************

const QList<NetworkNode::SPtr>& Network::getNodes() const
{
    return m_lNodes;
}


//*************************************************************************************************************

NetworkEdge::SPtr Network::getEdgeAt(int i)
{
    return m_lEdges.at(i);
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

QPair<float,float> Network::getMinMaxWeights() const
{
    return m_minMaxWeights;
}


//*************************************************************************************************************

QPair<float,float> Network::getMinMaxWeights(double dThresold) const
{
    float maxWeight = 0;
    float minWeight = 1000000000;

    for(int i = 0; i < m_lEdges.size(); ++i) {
        double weight = m_lEdges.at(i)->getWeight()(0,0);

        if(weight > maxWeight && weight > dThresold){
            maxWeight = weight;
        }

        if(weight < minWeight && weight > dThresold){
            minWeight = weight;
        }
    }

    return QPair<float,float>(minWeight,maxWeight);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxDegrees(double dThresold) const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getDegree(dThresold) > maxDegree){
            maxDegree = m_lNodes.at(i)->getDegree(dThresold);
        }

        if(m_lNodes.at(i)->getDegree(dThresold) < minDegree){
            minDegree = m_lNodes.at(i)->getDegree(dThresold);
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxIndegrees(double dThresold) const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getIndegree(dThresold) > maxDegree){
            maxDegree = m_lNodes.at(i)->getIndegree(dThresold);
        }

        if(m_lNodes.at(i)->getIndegree(dThresold) < minDegree){
            minDegree = m_lNodes.at(i)->getIndegree(dThresold);
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxOutdegrees(double dThresold) const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getOutdegree(dThresold) > maxDegree){
            maxDegree = m_lNodes.at(i)->getOutdegree(dThresold);
        }

        if(m_lNodes.at(i)->getOutdegree(dThresold) < minDegree){
            minDegree = m_lNodes.at(i)->getOutdegree(dThresold);
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

void Network::append(NetworkEdge::SPtr newEdge)
{
    if(newEdge->getWeight()(0,0) < m_minMaxWeights.first) {
        m_minMaxWeights.first = newEdge->getWeight()(0,0);
    }

    if(newEdge->getWeight()(0,0) > m_minMaxWeights.second) {
        m_minMaxWeights.second = newEdge->getWeight()(0,0);
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


//*************************************************************************************************************

MatrixXd Network::generateConnectMat(int idxRow, int idxCol) const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lEdges.size(); ++i)
    {
        int row = m_lEdges.at(i)->getStartNodeID();
        int col = m_lEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols())
        {
            matDist(row,col) = m_lEdges.at(i)->getWeight()(idxRow, idxCol);
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}





