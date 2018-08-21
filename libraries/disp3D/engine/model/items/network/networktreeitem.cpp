//=============================================================================================================
/**
* @file     networktreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
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
* @brief    NetworkTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "networktreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/networkmaterial.h"
#include "../../3dhelpers/custommesh.h"
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

#include <disp/plots/helpers/colormap.h>

#include <connectivity/network/networknode.h>
#include <connectivity/network/networkedge.h>

#include <fiff/fiff_types.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QSphereGeometry>
#include <Qt3DExtras/QCylinderGeometry>
#include <Qt3DCore/QTransform>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace CONNECTIVITYLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkTreeItem::NetworkTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: AbstractMeshTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}


//*************************************************************************************************************

void NetworkTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Network item");

    //Add meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    QVector3D vecEdgeTrehshold(0,5,10);
    if(!m_pItemNetworkThreshold) {
        m_pItemNetworkThreshold = new MetaTreeItem(MetaTreeItemTypes::DataThreshold,
                                                   QString("%1,%2,%3").arg(vecEdgeTrehshold.x()).arg(vecEdgeTrehshold.y()).arg(vecEdgeTrehshold.z()));
    }

    list << m_pItemNetworkThreshold;
    list << new QStandardItem(m_pItemNetworkThreshold->toolTip());
    this->appendRow(list);
    data.setValue(vecEdgeTrehshold);
    m_pItemNetworkThreshold->setData(data, MetaTreeItemRoles::DataThreshold);
    connect(m_pItemNetworkThreshold.data(), &MetaTreeItem::dataChanged,
            this, &NetworkTreeItem::onNetworkThresholdChanged);

    list.clear();
    MetaTreeItem* pItemNetworkMatrix = new MetaTreeItem(MetaTreeItemTypes::NetworkMatrix, "Show network matrix");
    list << pItemNetworkMatrix;
    list << new QStandardItem(pItemNetworkMatrix->toolTip());
    this->appendRow(list);

    //Set material
    NetworkMaterial* pNetworkMaterial = new NetworkMaterial();
    this->setMaterial(pNetworkMaterial);
}


//*************************************************************************************************************

void NetworkTreeItem::addData(const Network& tNetworkData)
{
    //Add data which is held by this NetworkTreeItem
    QVariant data;

    data.setValue(tNetworkData);
    this->setData(data, Data3DTreeModelItemRoles::Data);

    MatrixXd matDist = tNetworkData.getConnectivityMatrix();
    data.setValue(matDist);
    this->setData(data, Data3DTreeModelItemRoles::NetworkDataMatrix);

    //Plot network
    plotNetwork(tNetworkData,
                    m_pItemNetworkThreshold->data(MetaTreeItemRoles::DataThreshold).value<QVector3D>());
}


//*************************************************************************************************************

void NetworkTreeItem::setThresholds(const QVector3D& vecThresholds)
{
    if(m_pItemNetworkThreshold) {
        QVariant data;
        data.setValue(vecThresholds);
        m_pItemNetworkThreshold->setData(data, MetaTreeItemRoles::DataThreshold);

        QString sTemp = QString("%1,%2,%3").arg(vecThresholds.x()).arg(vecThresholds.y()).arg(vecThresholds.z());
        data.setValue(sTemp);
        m_pItemNetworkThreshold->setData(data, Qt::DisplayRole);
    }
}


//*************************************************************************************************************

void NetworkTreeItem::onNetworkThresholdChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        Network tNetwork = this->data(Data3DTreeModelItemRoles::Data).value<Network>();

        plotNetwork(tNetwork, vecThresholds.value<QVector3D>());
    }
}


//*************************************************************************************************************

void NetworkTreeItem::plotNetwork(const Network& tNetworkData, const QVector3D& vecThreshold)
{
    //Draw network nodes and edges
    plotNodes(tNetworkData, vecThreshold);
    plotEdges(tNetworkData, vecThreshold);
}


//*************************************************************************************************************

void NetworkTreeItem::plotNodes(const Network& tNetworkData,
                                const QVector3D& vecThreshold)
{
    if(tNetworkData.isEmpty()) {
        qDebug() << "NetworkTreeItem::plotNodes - Network data is empty. Returning.";
        return;
    }

    QList<NetworkNode::SPtr> lNetworkNodes = tNetworkData.getNodes();
    int iMaxDegree = tNetworkData.getMinMaxDegrees(vecThreshold.x()).second;

    if(!m_pNodesEntity) {
        m_pNodesEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pEdgesIn) {
        if(!m_pNodesGeometry) {
            m_pNodesGeometry = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
            m_pNodesGeometry->setRadius(0.006f);
        }

        m_pNodes = new GeometryMultiplier(m_pNodesGeometry);

        m_pNodesEntity->addComponent(m_pNodes);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial;
        pMaterial->setAmbient(Qt::blue);
        pMaterial->setAlpha(1.0f);
        m_pNodesEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each sphere instance
    QVector<QMatrix4x4> vTransforms;
    QVector<QColor> vColorsNodes;
    vTransforms.reserve(lNetworkNodes.size());
    QVector3D tempPos;
    int dDegree = 0;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        dDegree = lNetworkNodes.at(i)->getDegree(vecThreshold.x());

        tempPos = QVector3D(lNetworkNodes.at(i)->getVert()(0),
                            lNetworkNodes.at(i)->getVert()(1),
                            lNetworkNodes.at(i)->getVert()(2));

        //Set position and scale
        QMatrix4x4 tempTransform;

        tempTransform.translate(tempPos);
        tempTransform.scale((float)dDegree/(float)iMaxDegree);
        vTransforms.push_back(tempTransform);

        if(dDegree != 0.0f) {
            vColorsNodes.push_back(QColor(ColorMap::valueToJet((float)dDegree/(float)iMaxDegree)));
        } else {
            vColorsNodes.push_back(QColor(ColorMap::valueToJet(0.0f)));
        }
    }

    //Set instance Transform and colors
    if(!vTransforms.isEmpty() && !vColorsNodes.isEmpty()) {
        m_pNodes->setTransforms(vTransforms);
        m_pNodes->setColors(vColorsNodes);
    }
}


//*************************************************************************************************************

void NetworkTreeItem::plotEdges(const Network &tNetworkData,
                                const QVector3D& vecThreshold)
{
    if(tNetworkData.isEmpty()) {
        qDebug() << "NetworkTreeItem::plotEdges - Network data is empty. Returning.";
        return;
    }

    float fMaxWeight = tNetworkData.getMinMaxWeights(vecThreshold.x()).second;

    QList<NetworkNode::SPtr> lNetworkNodes = tNetworkData.getNodes();

    if(!m_pEdgeEntity) {
        m_pEdgeEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pEdgesIn) {
        if(!m_pEdgesInGeometry) {
            m_pEdgesInGeometry = QSharedPointer<Qt3DExtras::QCylinderGeometry>::create();
            m_pEdgesInGeometry->setRadius(0.0005f);
            m_pEdgesInGeometry->setLength(1.0f);
        }

        m_pEdgesIn = new GeometryMultiplier(m_pEdgesInGeometry);

        m_pEdgeEntity->addComponent(m_pEdgesIn);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial;
        pMaterial->setAmbient(Qt::red);
        pMaterial->setAlpha(0.8f);
        m_pEdgeEntity->addComponent(pMaterial);
    }

    if(!m_pEdgesOut) {
        if(!m_pEdgesOutGeometry) {
            m_pEdgesOutGeometry = QSharedPointer<Qt3DExtras::QCylinderGeometry>::create();
            m_pEdgesOutGeometry->setRadius(0.0005f);
            m_pEdgesOutGeometry->setLength(1.0f);
        }

        m_pEdgesOut = new GeometryMultiplier(m_pEdgesOutGeometry);

        m_pEdgeEntity->addComponent(m_pEdgesOut);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial;
        pMaterial->setAmbient(Qt::blue);
        pMaterial->setAlpha(0.8f);
        m_pEdgeEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each cylinder instance
    QVector<QMatrix4x4> vTransformsEdgesIn, vTransformsEdgesOut;
    QVector<QColor> vColorsEdgesIn, vColorsEdgesOut;
    QVector3D startPos, endPos, edgePos, diff;
    float fWeight = 0.0f;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        //Plot in edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesIn().size(); ++j) {
            RowVectorXf vectorStart = lNetworkNodes.at(lNetworkNodes.at(i)->getEdgesIn().at(j)->getStartNodeID())->getVert();
            startPos = QVector3D(vectorStart(0),
                                 vectorStart(1),
                                 vectorStart(2));

            RowVectorXf vectorEnd = lNetworkNodes.at(lNetworkNodes.at(i)->getEdgesIn().at(j)->getEndNodeID())->getVert();
            endPos = QVector3D(vectorEnd(0),
                                 vectorEnd(1),
                                 vectorEnd(2));

            fWeight = lNetworkNodes.at(i)->getEdgesIn().at(j)->getWeight()(0,0);
            if(std::fabs(fWeight) > vecThreshold.x() &&
                    startPos != endPos) {
                diff = endPos - startPos;
                edgePos = endPos - diff/2;

                QMatrix4x4 tempTransform;
                tempTransform.translate(edgePos);
                tempTransform.rotate(QQuaternion::rotationTo(QVector3D(0,1,0), diff.normalized()).normalized());
                tempTransform.scale(1.0,diff.length(),1.0);
                vTransformsEdgesIn.push_back(tempTransform);
                if(fMaxWeight != 0.0f) {
                    vColorsEdgesIn.push_back(QColor(ColorMap::valueToJet(fWeight/fMaxWeight)));
                } else {
                    vColorsEdgesIn.push_back(QColor(ColorMap::valueToJet(0.0f)));
                }
            }
        }

        //Plot out edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesOut().size(); ++j) {
            RowVectorXf vectorStart = lNetworkNodes.at(lNetworkNodes.at(i)->getEdgesOut().at(j)->getStartNodeID())->getVert();
            startPos = QVector3D(vectorStart(0),
                                 vectorStart(1),
                                 vectorStart(2));

            RowVectorXf vectorEnd = lNetworkNodes.at(lNetworkNodes.at(i)->getEdgesOut().at(j)->getEndNodeID())->getVert();
            endPos = QVector3D(vectorEnd(0),
                                 vectorEnd(1),
                                 vectorEnd(2));

//            qDebug() << "NetworkTreeItem::plotEdges weight " << lNetworkNodes.at(i)->getEdgesOut().at(j)->getWeight()(0,0);
//            qDebug() << "NetworkTreeItem::plotEdges threshold " << vecThreshold.x();

            fWeight = lNetworkNodes.at(i)->getEdgesOut().at(j)->getWeight()(0,0);
            if(std::fabs(fWeight) > vecThreshold.x() &&
                    startPos != endPos) {
                diff = endPos - startPos;
                edgePos = endPos - diff/2;

                QMatrix4x4 tempTransform;
                tempTransform.translate(edgePos);
                tempTransform.rotate(QQuaternion::rotationTo(QVector3D(0,1,0), diff.normalized()).normalized());
                tempTransform.scale(1.0,diff.length(),1.0);
                vTransformsEdgesOut.push_back(tempTransform);
                if(fMaxWeight != 0.0f) {
                    vColorsEdgesOut.push_back(QColor(ColorMap::valueToJet(fWeight/fMaxWeight)));
                } else {
                    vColorsEdgesOut.push_back(QColor(ColorMap::valueToJet(0.0f)));
                }
            }
        }
    }

    //Set instance transforms and colors
    if(!vTransformsEdgesIn.isEmpty() && !vColorsEdgesIn.isEmpty()) {
        m_pEdgesIn->setTransforms(vTransformsEdgesIn);
        m_pEdgesIn->setColors(vColorsEdgesIn);
    }

    if(!vTransformsEdgesOut.isEmpty() && !vColorsEdgesOut.isEmpty()) {
        m_pEdgesOut->setTransforms(vTransformsEdgesOut);
        m_pEdgesOut->setColors(vColorsEdgesOut);
    }
}


