//=============================================================================================================
/**
 * @file     network.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Faris Yahya <mfarisyahya@gmail.com>
 * @since    0.1.0
 * @date     July, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lorenz Esch, Faris Yahya. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "network.h"

#include "networkedge.h"
#include "networknode.h"

#include <utils/spectral.h>

#include <limits>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace CONNECTIVITYLIB;
using namespace Eigen;
using namespace UTILSLIB;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Network::Network(const QString& sConnectivityMethod,
                 double dThreshold)
: m_sConnectivityMethod(sConnectivityMethod)
, m_minMaxFullWeights(QPair<double,double>(std::numeric_limits<double>::max(),0.0))
, m_minMaxThresholdedWeights(QPair<double,double>(std::numeric_limits<double>::max(),0.0))
, m_dThreshold(dThreshold)
, m_fSFreq(0.0f)
, m_iFFTSize(128)
, m_iNumberFreqBins(0)
{
    qRegisterMetaType<CONNECTIVITYLIB::Network>("CONNECTIVITYLIB::Network");
    qRegisterMetaType<CONNECTIVITYLIB::Network::SPtr>("CONNECTIVITYLIB::Network::SPtr");
    qRegisterMetaType<QList<CONNECTIVITYLIB::Network> >("QList<CONNECTIVITYLIB::Network>");
    qRegisterMetaType<QList<CONNECTIVITYLIB::Network::SPtr> >("QList<CONNECTIVITYLIB::Network::SPtr>");
}

//=============================================================================================================

MatrixXd Network::getFullConnectivityMatrix(bool bGetMirroredVersion) const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        int row = m_lFullEdges.at(i)->getStartNodeID();
        int col = m_lFullEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols()) {
            matDist(row,col) = m_lFullEdges.at(i)->getWeight();

            if(bGetMirroredVersion) {
                matDist(col,row) = m_lFullEdges.at(i)->getWeight();
            }
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}

//=============================================================================================================

MatrixXd Network::getThresholdedConnectivityMatrix(bool bGetMirroredVersion) const
{
    MatrixXd matDist(m_lNodes.size(), m_lNodes.size());
    matDist.setZero();

    for(int i = 0; i < m_lThresholdedEdges.size(); ++i) {
        int row = m_lThresholdedEdges.at(i)->getStartNodeID();
        int col = m_lThresholdedEdges.at(i)->getEndNodeID();

        if(row < matDist.rows() && col < matDist.cols()) {
            matDist(row,col) = m_lThresholdedEdges.at(i)->getWeight();

            if(bGetMirroredVersion) {
                matDist(col,row) = m_lThresholdedEdges.at(i)->getWeight();
            }
        }
    }

    //IOUtils::write_eigen_matrix(matDist,"eigen.txt");
    return matDist;
}

//=============================================================================================================

const QList<NetworkEdge::SPtr>& Network::getFullEdges() const
{
    return m_lFullEdges;
}

//=============================================================================================================

const QList<NetworkEdge::SPtr>& Network::getThresholdedEdges() const
{
    return m_lThresholdedEdges;
}

//=============================================================================================================

const QList<NetworkNode::SPtr>& Network::getNodes() const
{
    return m_lNodes;
}

//=============================================================================================================

NetworkNode::SPtr Network::getNodeAt(int i)
{
    return m_lNodes.at(i);
}

//=============================================================================================================

qint16 Network::getFullDistribution() const
{
    qint16 distribution = 0;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        distribution += m_lNodes.at(i)->getFullDegree();
    }

    return distribution;
}

//=============================================================================================================

qint16 Network::getThresholdedDistribution() const
{
    qint16 distribution = 0;

    for(int i = 0; i < m_lNodes.size(); ++i) {
        distribution += m_lNodes.at(i)->getThresholdedDegree();
    }

    return distribution;
}

//=============================================================================================================

void Network::setConnectivityMethod(const QString& sConnectivityMethod)
{
    m_sConnectivityMethod = sConnectivityMethod;
}

//=============================================================================================================

QString Network::getConnectivityMethod() const
{
    return m_sConnectivityMethod;
}

//=============================================================================================================

QPair<double, double> Network::getMinMaxFullWeights() const
{
    return m_minMaxFullWeights;
}

//=============================================================================================================

QPair<double, double> Network::getMinMaxThresholdedWeights() const
{
    return m_minMaxThresholdedWeights;
}

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

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

//=============================================================================================================

void Network::setThreshold(double dThreshold)
{
    m_dThreshold = dThreshold;
    m_lThresholdedEdges.clear();

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        if(fabs(m_lFullEdges.at(i)->getWeight()) >= m_dThreshold) {
            m_lFullEdges.at(i)->setActive(true);
            m_lThresholdedEdges.append(m_lFullEdges.at(i));
        } else {
            m_lFullEdges.at(i)->setActive(false);
        }
    }

    m_minMaxThresholdedWeights.first = m_dThreshold;
    m_minMaxThresholdedWeights.second = m_minMaxFullWeights.second;
}

//=============================================================================================================

double Network::getThreshold()
{
    return m_dThreshold;
}

//=============================================================================================================

void Network::setFrequencyRange(float fLowerFreq, float fUpperFreq)
{
    if(fLowerFreq > fUpperFreq || fUpperFreq < fLowerFreq) {
        qDebug() << "Network::setFrequencyRange - Upper and lower frequency are out of range from each other. Weights will not be recalculated. Returning.";
        return;
    }

    if(m_fSFreq <= 0.0f) {
        qDebug() << "Network::setFrequencyRange - Sampling frequency has not been set. Returning.";
        return;
    }

    if(fUpperFreq > m_fSFreq/2.0f) {
        qDebug() << "Network::setFrequencyRange - Upper frequency is bigger than nyquist frequency. Returning.";
        return;
    }

    if(m_iNumberFreqBins <= 0) {
        qDebug() << "Network::setFrequencyRange - Number of samples has not been set. Returning.";
        return;
    }

    double dScaleFactor = m_iFFTSize/(m_fSFreq/2);

    m_minMaxFrequency.first = fLowerFreq;
    m_minMaxFrequency.second = fUpperFreq;

    int iLowerBin = fLowerFreq * dScaleFactor;
    int iUpperBin = fUpperFreq * dScaleFactor;

    // Update the min max values
    m_minMaxFullWeights = QPair<double,double>(std::numeric_limits<double>::max(),0.0);

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        m_lFullEdges.at(i)->setFrequencyBins(QPair<int,int>(iLowerBin,iUpperBin));

        if(fabs(m_lFullEdges.at(i)->getWeight()) < m_minMaxFullWeights.first) {
            m_minMaxFullWeights.first = fabs(m_lFullEdges.at(i)->getWeight());
        } else if(fabs(m_lFullEdges.at(i)->getWeight()) > m_minMaxFullWeights.second) {
            m_minMaxFullWeights.second = fabs(m_lFullEdges.at(i)->getWeight());
        }
    }
}

//=============================================================================================================

const QPair<float,float>& Network::getFrequencyRange() const
{
    return m_minMaxFrequency;
}

//=============================================================================================================

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

        if(fabs(newEdge->getWeight()) >= m_dThreshold) {
            m_lThresholdedEdges << newEdge;
        }
    }
}

//=============================================================================================================

void Network::append(NetworkNode::SPtr newNode)
{
    m_lNodes << newNode;
}

//=============================================================================================================

bool Network::isEmpty() const
{
    if(m_lFullEdges.isEmpty() || m_lNodes.isEmpty()) {
        return true;
    }

    return false;
}

//=============================================================================================================

void Network::normalize()
{
    // Normalize full network
    if(m_minMaxFullWeights.second == 0.0) {
        qDebug() << "Network::normalize() - Max weight is 0. Returning.";
        return;
    }

    for(int i = 0; i < m_lFullEdges.size(); ++i) {
        m_lFullEdges.at(i)->setWeight(m_lFullEdges.at(i)->getWeight()/m_minMaxFullWeights.second);
    }

    m_minMaxFullWeights.first = m_minMaxFullWeights.first/m_minMaxFullWeights.second;
    m_minMaxFullWeights.second = 1.0;

    m_minMaxThresholdedWeights.first = m_minMaxThresholdedWeights.first/m_minMaxThresholdedWeights.second;
    m_minMaxThresholdedWeights.second = 1.0;
}

//=============================================================================================================

VisualizationInfo Network::getVisualizationInfo() const
{
    return m_visualizationInfo;
}

//=============================================================================================================

void Network::setVisualizationInfo(const VisualizationInfo& visualizationInfo)
{
    m_visualizationInfo = visualizationInfo;
}

//=============================================================================================================

float Network::getSamplingFrequency() const
{
    return m_fSFreq;
}

//=============================================================================================================

void Network::setSamplingFrequency(float fSFreq)
{
    m_fSFreq = fSFreq;
}

//=============================================================================================================

int Network::getUsedFreqBins() const
{
    return m_iNumberFreqBins;
}

//=============================================================================================================

void Network::setUsedFreqBins(int iNumberFreqBins)
{
    m_iNumberFreqBins = iNumberFreqBins;
}

//=============================================================================================================

void Network::setFFTSize(int iFFTSize)
{
    m_iFFTSize = iFFTSize;
}

//=============================================================================================================

int Network::getFFTSize()
{
    return m_iFFTSize;
}

