//=============================================================================================================
/**
* @file     network.cpp
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017 Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Network class implementation.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "network.h"
#include "node.h"
#include "edge.h"


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================



//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Network::Network(QObject *parent)
: QObject(parent)
, m_penStyle(Qt::SolidLine)
, m_weightThreshold(0.1f)
, m_weightStrength(5)
{

}


//*************************************************************************************************************

Network::Network(CNTK::FunctionPtr model, QObject *parent)
: QObject(parent)
, m_pModel(model)
, m_penStyle(Qt::SolidLine)
, m_weightThreshold(0.2f)
, m_weightStrength(5)
{
    generateNetwork();
}


//*************************************************************************************************************

void Network::setModel(CNTK::FunctionPtr &model)
{
    m_pModel = model;
    // TODO Update network
}


//*************************************************************************************************************

QList<QList<Node *> > Network::layerNodes() const
{
    return m_listLayerNodes;
}


//*************************************************************************************************************

void Network::setLayerNodes(const QList<QList<Node *> > &listLayerNodes)
{
    m_listLayerNodes = listLayerNodes;
}


//*************************************************************************************************************

QList<QList<Edge *> > Network::edges() const
{
    return m_listEdges;
}


//*************************************************************************************************************

void Network::setEdges(const QList<QList<Edge *> > &listEdges)
{
    m_listEdges = listEdges;
}


//*************************************************************************************************************

bool Network::isSetup() const
{
    return true;
}


//*************************************************************************************************************

void Network::setSolidLine()
{
    if(m_penStyle != Qt::SolidLine) {
        m_penStyle = Qt::SolidLine;
        emit update_signal();
    }
}


//*************************************************************************************************************

void Network::setDashLine()
{
    if(m_penStyle != Qt::DashLine) {
        m_penStyle = Qt::DashLine;
        emit update_signal();
    }
}


//*************************************************************************************************************

void Network::setDotLine()
{
    if(m_penStyle != Qt::DotLine) {
        m_penStyle = Qt::DotLine;
        emit update_signal();
    }
}


//*************************************************************************************************************

void Network::setWeightThreshold(int thr)
{
    m_weightThreshold = static_cast<float>(thr) / 1000.0f;
    emit updateWeightThreshold_signal();
}


//*************************************************************************************************************

void Network::setWeightStrength(int strength)
{
    m_weightStrength = strength;
    emit updateWeightStrength_signal();
}


//*************************************************************************************************************

void Network::updateWeights()
{
    // Update Weights

    if(!m_pModel)
        return;

    //
    // Analyze CNTK Model Structure
    //
    QVector<int> layerDim;
    QVector<MatrixXf> vecWeights;
    int inDim = 0;
    int outDim = 0;

    MatrixXf weights;
    VectorXf bias;
    int bufferCount;

    size_t i = m_pModel->Parameters().size() - 1;
    for (int k = 0; k < static_cast<int>(m_pModel->Parameters().size()); ++k, --i) {
        QString param = QString::fromStdWString(m_pModel->Parameters()[i].Shape().AsString());

        if(param.contains(" x ")) {
            param.replace(QString("["), QString(""));param.replace(QString("]"), QString(""));
            QStringList dimensions = param.split(" x ");
            outDim = dimensions[0].toInt();
            inDim = dimensions[1].toInt();

            weights.resize(outDim,inDim);
            bufferCount = 0;
            for(int m = 0; m < outDim; ++m) {
                for(int n = 0; n < inDim; ++n) {
                    weights(m,n) = m_pModel->Parameters()[i].Value()->DataBuffer<float>()[bufferCount];
                    ++bufferCount;
                }
            }

            // ToDo put in one class
            layerDim.append(inDim);
            vecWeights.append(weights);
        }
    }
    layerDim.append(outDim);

    //
    // Create items according to the dimensions
    //

    for(int layer = 0; layer < layerDim.size(); ++layer) {


        // Create Edges
        if(layer - 1 >= 0) {

            // Dimension check
            if( vecWeights[layer-1].rows() != m_listLayerNodes[layer].size() && vecWeights[layer-1].cols() != m_listLayerNodes[layer-1].size() ) {
                qCritical("Dimensions do not match.\n");
                return;
//                qDebug() << "Dimension Check" << vecWeights[layer-1].rows() << "x" << vecWeights[layer-1].cols();
//                qDebug() << "Check" << m_listNodes[layer].size() << "x" << m_listNodes[layer-1].size();
            }

            int count = 0;

            for(int i = 0; i < m_listLayerNodes[layer-1].size(); ++i ) {
                for(int j = 0; j < m_listLayerNodes[layer].size(); ++j ) {
                    m_listEdges[layer-1][count]->setWeight(vecWeights[layer-1](j,i));
                    ++count;
                }
            }
        }
    }
}


//*************************************************************************************************************

void Network::generateNetwork()
{

    if(!m_pModel)
        return;

    //
    // Analyze CNTK Model Structure
    //
    QVector<int> layerDim;
    QVector<MatrixXf> vecWeights;
    int inDim = 0;
    int outDim = 0;

    MatrixXf weights;
    VectorXf bias;
    int bufferCount;

    size_t i = m_pModel->Parameters().size() - 1;
    for (int k = 0; k < static_cast<int>(m_pModel->Parameters().size()); ++k, --i) {
        fprintf(stderr,"\n >> Level = %ju <<\n",m_pModel->Parameters().size() - i);
        fprintf(stderr,"Dim: %ls\n",m_pModel->Parameters()[i].Shape().AsString().c_str());


        fprintf(stderr,"Value Dim: %ls\n",m_pModel->Parameters()[i].Value()->Shape().AsString().c_str());

        QString param = QString::fromStdWString(m_pModel->Parameters()[i].Shape().AsString());

        if(param.contains(" x ")) {
            param.replace(QString("["), QString(""));param.replace(QString("]"), QString(""));
            QStringList dimensions = param.split(" x ");
            outDim = dimensions[0].toInt();
            inDim = dimensions[1].toInt();

            weights.resize(outDim,inDim);
            bufferCount = 0;
            for(int m = 0; m < outDim; ++m) {
                for(int n = 0; n < inDim; ++n) {
                    weights(m,n) = m_pModel->Parameters()[i].Value()->DataBuffer<float>()[bufferCount];
                    ++bufferCount;
                }
            }

            // ToDo put in one class
            layerDim.append(inDim);
            vecWeights.append(weights);
        }
    }
    layerDim.append(outDim);

    //
    // Create items according to the dimensions
    //
    int numLayers = layerDim.size();

    double layerDist = 400.0;
    double nodeDist = 50.0;

    double x_root = -((numLayers-1.0)*layerDist) / 2.0;

    QList<Node*> listCurrentLayer;
    QList<Edge*> listCurrentEdges;
    QPointF layerRoot, currentPos;

    for(int layer = 0; layer < layerDim.size(); ++layer) {
        layerRoot = QPointF( x_root + layer*layerDist, - (layerDim[layer]/2) * nodeDist);

        // Create Nodes
        for(int i = 0; i < layerDim[layer]; ++i ) {
            listCurrentLayer.append(new Node(this));
            currentPos = layerRoot + QPointF(0,nodeDist * i);
            listCurrentLayer[i]->setPos(currentPos);
        }
        m_listLayerNodes.append(listCurrentLayer);
        listCurrentLayer.clear();

        // Create Edges
        if(layer - 1 >= 0) {

            // Dimension check
            if( vecWeights[layer-1].rows() != m_listLayerNodes[layer].size() && vecWeights[layer-1].cols() != m_listLayerNodes[layer-1].size() ) {
                qCritical("Dimensions do not match.\n");
                return;
//                qDebug() << "Dimension Check" << vecWeights[layer-1].rows() << "x" << vecWeights[layer-1].cols();
//                qDebug() << "Check" << m_listNodes[layer].size() << "x" << m_listNodes[layer-1].size();
            }

            for(int i = 0; i < m_listLayerNodes[layer-1].size(); ++i ) {
                for(int j = 0; j < m_listLayerNodes[layer].size(); ++j ) {
                    listCurrentEdges.append(new Edge(this, m_listLayerNodes[layer-1][i], m_listLayerNodes[layer][j]));

                    listCurrentEdges.last()->setWeight(vecWeights[layer-1](j,i));
                }
            }
            m_listEdges.append(listCurrentEdges);
            listCurrentEdges.clear();
        }
    }
}
