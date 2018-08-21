//=============================================================================================================
/**
* @file     networknode.cpp
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
* @brief    NetworkNode class definition.
*
*/


//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networknode.h"

#include "networkedge.h"


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE GLOBAL METHODS
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkNode::NetworkNode(qint16 iId, const RowVectorXf& vecVert)
: m_bIsHub(false)
, m_iId(iId)
, m_vecVert(vecVert)
{
}


//*************************************************************************************************************

const QList<QSharedPointer<NetworkEdge> >& NetworkNode::getEdgesIn() const
{
    return m_lEdgesIn;
}


//*************************************************************************************************************

const QList<QSharedPointer<NetworkEdge> >& NetworkNode::getEdgesOut() const
{
    return m_lEdgesOut;
}


//*************************************************************************************************************

int NetworkNode::getNumberEdges() const
{
    return m_lEdgesIn.size() + m_lEdgesOut.size();
}


//*************************************************************************************************************

const RowVectorXf& NetworkNode::getVert() const
{
    return m_vecVert;
}


//*************************************************************************************************************

qint16 NetworkNode::getId() const
{
    return m_iId;
}


//*************************************************************************************************************

qint16 NetworkNode::getDegree(double dThresold) const
{
    if(dThresold == 0.0) {
        return m_lEdgesIn.size() + m_lEdgesOut.size();
    }

    int degree = 0;

    for(int i = 0; i< m_lEdgesIn.size(); i++) {
        if(m_lEdgesIn.at(i)->getWeight()(0,0) >= dThresold) {
            degree++;
        }
    }

    for(int i = 0; i< m_lEdgesOut.size(); i++) {
        if(m_lEdgesOut.at(i)->getWeight()(0,0) >= dThresold) {
            degree++;
        }
    }

    return degree;
}


//*************************************************************************************************************

qint16 NetworkNode::getIndegree(double dThresold) const
{
    if(dThresold == 0.0) {
        return m_lEdgesIn.size();
    }

    int degree = 0;

    for(int i = 0; i< m_lEdgesIn.size(); i++) {
        if(m_lEdgesIn.at(i)->getWeight()(0,0) >= dThresold) {
            degree++;
        }
    }

    return degree;
}


//*************************************************************************************************************

qint16 NetworkNode::getOutdegree(double dThresold) const
{
    if(dThresold == 0.0) {
        return m_lEdgesOut.size();
    }

    int degree = 0;

    for(int i = 0; i< m_lEdgesOut.size(); i++) {
        if(m_lEdgesOut.at(i)->getWeight()(0,0) >= dThresold) {
            degree++;
        }
    }

    return degree;
}


//*************************************************************************************************************

MatrixXd NetworkNode::getStrength() const
{
    MatrixXd matStrength;

    for(int i = 0; i < m_lEdgesIn.size(); ++i) {
        if(matStrength.size() == 0) {
            matStrength = m_lEdgesIn.at(i)->getWeight().setZero();
        }

        matStrength += m_lEdgesIn.at(i)->getWeight();
    }

    for(int i = 0; i < m_lEdgesOut.size(); ++i) {
        if(matStrength.size() == 0) {
            matStrength = m_lEdgesOut.at(i)->getWeight().setZero();
        }

        matStrength += m_lEdgesOut.at(i)->getWeight();
    }

    return matStrength;
}


//*************************************************************************************************************

MatrixXd NetworkNode::getInstrength() const
{
    MatrixXd matStrength;

    for(int i = 0; i < m_lEdgesIn.size(); ++i) {
        if(matStrength.size() == 0) {
            matStrength = m_lEdgesIn.at(i)->getWeight().setZero();
        }

        matStrength += m_lEdgesIn.at(i)->getWeight();
    }

    return matStrength;
}


//*************************************************************************************************************

MatrixXd NetworkNode::getOutstrength() const
{
    MatrixXd matStrength;

    for(int i = 0; i < m_lEdgesOut.size(); ++i) {
        if(matStrength.size() == 0) {
            matStrength = m_lEdgesOut.at(i)->getWeight().setZero();
        }

        matStrength += m_lEdgesOut.at(i)->getWeight();
    }

    return matStrength;
}


//*************************************************************************************************************

void NetworkNode::setHubStatus(bool bIsHub)
{
    m_bIsHub = bIsHub;
}


//*************************************************************************************************************

bool NetworkNode::getHubStatus() const
{
    return m_bIsHub;
}


//*************************************************************************************************************

void NetworkNode::append(QSharedPointer<NetworkEdge> newEdge)
{
    if(newEdge->getEndNodeID() == newEdge->getStartNodeID() ) {
        return;
    }

    if(newEdge->getEndNodeID() == this->getId()) {
        m_lEdgesIn << newEdge;
    }

    if(newEdge->getStartNodeID() == this->getId()) {
        m_lEdgesOut << newEdge;
    }
}



