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

#include <utils/spectral.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>


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
, m_minMaxFullWeights(QPair<double,double>(100000000000.0,0.0))
, m_minMaxThresholdedWeights(QPair<double,double>(100000000000.0,0.0))
, m_dThreshold(dThreshold)
{
    qRegisterMetaType<CONNECTIVITYLIB::Network>("CONNECTIVITYLIB::Network");
}


//*************************************************************************************************************

MatrixXd Network::getFullConnectivityMatrix() const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        int row = m_lFullEdges.at(i)->getStartNodeID();
        int col = m_lFullEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols()) {
            matDist(row,col) = m_lFullEdges.at(i)->getWeight();
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}


//*************************************************************************************************************

MatrixXd Network::getThresholdedConnectivityMatrix() const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lThresholdedEdges.size(); ++i) {
        int row = m_lThresholdedEdges.at(i)->getStartNodeID();
        int col = m_lThresholdedEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols()) {
            matDist(row,col) = m_lThresholdedEdges.at(i)->getWeight();
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}

//*************************************************************************************************************

const QList<NetworkEdge::SPtr>& Network::getFullEdges() const
{
    return m_lFullEdges;
}


//*************************************************************************************************************

const QList<NetworkEdge::SPtr>& Network::getThresholdedEdges() const
{
    return m_lThresholdedEdges;
}


//*************************************************************************************************************

const QList<NetworkNode::SPtr>& Network::getNodes() const
{
    return m_lNodes;
}


//*************************************************************************************************************

NetworkNode::SPtr Network::getNodeAt(int i)
{
    return m_lNodes.at(i);
}


//*************************************************************************************************************

qint16 Network::getFullDistribution() const
{
    qint16 distribution = 0;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        distribution += m_lNodes.at(i)->getFullDegree();
    }

    return distribution;
}


//*************************************************************************************************************

qint16 Network::getThresholdedDistribution() const
{
    qint16 distribution = 0;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        distribution += m_lNodes.at(i)->getThresholdedDegree();
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

QPair<double, double> Network::getMinMaxFullWeights() const
{
    return m_minMaxFullWeights;
}


//*************************************************************************************************************

QPair<double, double> Network::getMinMaxThresholdedWeights() const
{
    return m_minMaxThresholdedWeights;
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxFullDegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getFullDegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getFullDegree();
        } else if (m_lNodes.at(i)->getFullDegree() < minDegree){
            minDegree = m_lNodes.at(i)->getFullDegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxThresholdedDegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getThresholdedDegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getThresholdedDegree();
        } else if (m_lNodes.at(i)->getThresholdedDegree() < minDegree){
            minDegree = m_lNodes.at(i)->getThresholdedDegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxFullIndegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getFullIndegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getFullIndegree();
        } else if (m_lNodes.at(i)->getFullIndegree() < minDegree){
            minDegree = m_lNodes.at(i)->getFullIndegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxThresholdedIndegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getThresholdedIndegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getThresholdedIndegree();
        } else if (m_lNodes.at(i)->getThresholdedIndegree() < minDegree){
            minDegree = m_lNodes.at(i)->getThresholdedIndegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxFullOutdegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getFullOutdegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getFullOutdegree();
        } else if (m_lNodes.at(i)->getFullOutdegree() < minDegree){
            minDegree = m_lNodes.at(i)->getFullOutdegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

QPair<int,int> Network::getMinMaxThresholdedOutdegrees() const
{
    int maxDegree = 0;
    int minDegree = 1000000;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        if(m_lNodes.at(i)->getThresholdedOutdegree() > maxDegree){
            maxDegree = m_lNodes.at(i)->getThresholdedOutdegree();
        } else if (m_lNodes.at(i)->getThresholdedOutdegree() < minDegree){
            minDegree = m_lNodes.at(i)->getThresholdedOutdegree();
        }
    }

    return QPair<int,int>(minDegree,maxDegree);
}


//*************************************************************************************************************

void Network::setThreshold(double dThreshold)
{
    m_dThreshold = dThreshold;
    m_lThresholdedEdges.clear();

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        if(m_lFullEdges.at(i)->getWeight() >= m_dThreshold) {
            m_lFullEdges.at(i)->setActive(true);
            m_lThresholdedEdges.append(m_lFullEdges.at(i));
        } else {
            m_lFullEdges.at(i)->setActive(false);
        }
    }

    m_minMaxThresholdedWeights.first = m_dThreshold;
    m_minMaxThresholdedWeights.second = m_minMaxFullWeights.second;
}


//*************************************************************************************************************

double Network::getThreshold()
{
    return m_dThreshold;
}


//*************************************************************************************************************

void Network::setFrequencyBins(int iLowerBin, int iUpperBin)
{
    m_minMaxFrequencyBins.first = iLowerBin;
    m_minMaxFrequencyBins.second = iUpperBin;

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        m_lFullEdges.at(i)->setFrequencyBins(QPair<int,int>(iLowerBin,iUpperBin));
    }
}


//*************************************************************************************************************

const QPair<int,int>& Network::getFrequencyBins()
{
    return m_minMaxFrequencyBins;
}


//*************************************************************************************************************

void Network::append(NetworkEdge::SPtr newEdge)
{
    if(newEdge->getEndNodeID() != newEdge->getStartNodeID()) {
        double dEdgeWeight = newEdge->getWeight();
        if(dEdgeWeight < m_minMaxFullWeights.first) {
            m_minMaxFullWeights.first = dEdgeWeight;
        } else if(dEdgeWeight >= m_minMaxFullWeights.second) {
            m_minMaxFullWeights.second = dEdgeWeight;
        }

        m_lFullEdges << newEdge;
    }
}


//*************************************************************************************************************

void Network::append(NetworkNode::SPtr newNode)
{
    m_lNodes << newNode;
}


//*************************************************************************************************************

bool Network::isEmpty() const
{
    if(m_lFullEdges.isEmpty() || m_lNodes.isEmpty()) {
        return true;
    }

    return false;
}

