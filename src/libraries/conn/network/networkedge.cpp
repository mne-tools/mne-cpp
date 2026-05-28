//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2016-2026 MNE-CPP Authors
 *
 * @file     networkedge.cpp
 * @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
 * @since    0.1.0
 * @date     August 2016
 * @brief    Implementation of @ref CONNLIB::NetworkEdge - directional weighted edge with per-frequency weight matrix and configurable band-averaging window.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networkedge.h"

#include "networknode.h"

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

using namespace CONNLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkEdge::NetworkEdge(int iStartNodeID,
                         int iEndNodeID,
                         const MatrixXd& matWeight,
                         bool bIsActive,
                         int iStartWeightBin,
                         int iEndWeightBin)
: m_iStartNodeID(iStartNodeID)
, m_iEndNodeID(iEndNodeID)
, m_bIsActive(bIsActive)
, m_iMinMaxFreqBins(QPair<int,int>(iStartWeightBin,iEndWeightBin))
, m_dAveragedWeight(0.0)
{
    if(matWeight.rows() == 0 || matWeight.cols() == 0) {
        m_matWeight = MatrixXd::Zero(1,1);
        qDebug() << "NetworkEdge::NetworkEdge - Matrix weights number of rows and/or columns are zero. Setting to 1x1 zero matrix.";
    } else {
        m_matWeight = matWeight;
    }

    calculateAveragedWeight();
}

//=============================================================================================================

int NetworkEdge::getStartNodeID()
{
    return m_iStartNodeID;
}

//=============================================================================================================

int NetworkEdge::getEndNodeID()
{
    return m_iEndNodeID;
}

//=============================================================================================================

void NetworkEdge::setActive(bool bActiveFlag)
{
    m_bIsActive = bActiveFlag;
}

//=============================================================================================================

bool NetworkEdge::isActive()
{
    return m_bIsActive;
}

//=============================================================================================================

double NetworkEdge::getWeight() const
{
    return m_dAveragedWeight;
}

//=============================================================================================================

MatrixXd NetworkEdge::getMatrixWeight() const
{
    return m_matWeight;
}

//=============================================================================================================

void NetworkEdge::setWeight(double dAveragedWeight)
{
    m_dAveragedWeight = dAveragedWeight;
}

//=============================================================================================================

void NetworkEdge::calculateAveragedWeight()
{
    int iStartWeightBin = m_iMinMaxFreqBins.first;
    int iEndWeightBin = m_iMinMaxFreqBins.second;

    if(iEndWeightBin < iStartWeightBin || iStartWeightBin < -1 || iEndWeightBin < -1 ) {
        return;
    }

    int rows = m_matWeight.rows();

    if ((iEndWeightBin == -1 && iStartWeightBin == -1) ) {
        m_dAveragedWeight = m_matWeight.mean();
    } else if(iStartWeightBin < rows) {
        if(iEndWeightBin < rows) {
            m_dAveragedWeight = m_matWeight.block(iStartWeightBin,0,iEndWeightBin-iStartWeightBin+1,1).mean();
        } else {
            m_dAveragedWeight = m_matWeight.block(iStartWeightBin,0,rows-iStartWeightBin,1).mean();
        }
    }
}

//=============================================================================================================

void NetworkEdge::setFrequencyBins(const QPair<int,int>& minMaxFreqBins)
{
    m_iMinMaxFreqBins = minMaxFreqBins;

    if(m_iMinMaxFreqBins.second < m_iMinMaxFreqBins.first || m_iMinMaxFreqBins.first < -1 || m_iMinMaxFreqBins.second < -1 ) {
        return;
    }

    calculateAveragedWeight();
}

//=============================================================================================================

const QPair<int,int>& NetworkEdge::getFrequencyBins()
{
    return m_iMinMaxFreqBins;
}

