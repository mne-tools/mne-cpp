//=============================================================================================================
/**
 * @file     networkedge.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2016
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
 * @brief    NetworkEdge class definition.
 *
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

using namespace CONNECTIVITYLIB;
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

