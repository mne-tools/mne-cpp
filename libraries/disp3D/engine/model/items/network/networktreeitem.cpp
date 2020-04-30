//=============================================================================================================
/**
 * @file     networktreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     January, 2016
 *
 * @section  LICENSE
 *
 * Copyright (C) 2016, Lars Debor, Lorenz Esch. All rights reserved.
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

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <Qt3DExtras/QSphereGeometry>
#include <Qt3DExtras/QCylinderGeometry>
#include <Qt3DCore/QTransform>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace Eigen;
using namespace MNELIB;
using namespace DISP3DLIB;
using namespace CONNECTIVITYLIB;
using namespace DISPLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkTreeItem::NetworkTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: Abstract3DTreeItem(p3DEntityParent, iType, text)
{
    initItem();
}

//=============================================================================================================

void NetworkTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Network item");

    //Add meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    QVector3D vecEdgeTrehshold(0,0.5,1);
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

    MetaTreeItem* pItemColormapType = new MetaTreeItem(MetaTreeItemTypes::ColormapType, "Viridis");
    connect(pItemColormapType, &MetaTreeItem::dataChanged,
            this, &NetworkTreeItem::onColormapTypeChanged);
    list.clear();
    list << pItemColormapType;
    list << new QStandardItem(pItemColormapType->toolTip());
    this->appendRow(list);
    data.setValue(QString("Hot"));
    pItemColormapType->setData(data, MetaTreeItemRoles::ColormapType);

    list.clear();
    MetaTreeItem* pItemNetworkMatrix = new MetaTreeItem(MetaTreeItemTypes::NetworkMatrix, "Show network matrix");
    list << pItemNetworkMatrix;
    list << new QStandardItem(pItemNetworkMatrix->toolTip());
    this->appendRow(list);

//    //Set material
//    NetworkMaterial* pNetworkMaterial = new NetworkMaterial();
//    this->setMaterial(pNetworkMaterial);
}

//=============================================================================================================

void NetworkTreeItem::addData(const Network& tNetworkData)
{
    //Add data which is held by this NetworkTreeItem
    QVariant data;

    Network tNetwork = tNetworkData;
    tNetwork.setThreshold(m_pItemNetworkThreshold->data(MetaTreeItemRoles::DataThreshold).value<QVector3D>().x());

    data.setValue(tNetwork);
    this->setData(data, Data3DTreeModelItemRoles::NetworkData);

    MatrixXd matDist = tNetwork.getFullConnectivityMatrix();
//  MatrixXd matDist = tNetwork.getThresholdedConnectivityMatrix();
    data.setValue(matDist);
    this->setData(data, Data3DTreeModelItemRoles::Data);

    //Plot network
    if(this->checkState() == Qt::Checked) {
        plotNetwork(tNetwork);
    }
}

//=============================================================================================================

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

//=============================================================================================================

void NetworkTreeItem::onNetworkThresholdChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        Network tNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network>();
        addData(tNetwork);
    }
}

//=============================================================================================================

void NetworkTreeItem::onColorChanged(const QVariant& color)
{
    if(color.canConvert<QColor>()) {
        QColor networkColor = color.value<QColor>();

        Network tNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network>();
        VisualizationInfo info = tNetwork.getVisualizationInfo();
        info.sMethod = "Color";
        info.colEdges = Vector4i(networkColor.red(),
                                 networkColor.green(),
                                 networkColor.blue(),
                                 networkColor.alpha());
        info.colNodes = Vector4i(networkColor.red(),
                                 networkColor.green(),
                                 networkColor.blue(),
                                 networkColor.alpha());
        tNetwork.setVisualizationInfo(info);

        QVariant data;
        data.setValue(tNetwork);

        this->setData(data, Data3DTreeModelItemRoles::NetworkData);

        plotNetwork(tNetwork);
    }
}

//=============================================================================================================

void NetworkTreeItem::onColormapTypeChanged(const QVariant& sColormapType)
{
    if(sColormapType.canConvert<QString>()) {
        Network tNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network>();
        VisualizationInfo info = tNetwork.getVisualizationInfo();
        info.sMethod = "Map";
        info.sColormap = sColormapType.toString();
        tNetwork.setVisualizationInfo(info);

        QVariant data;
        data.setValue(tNetwork);

        this->setData(data, Data3DTreeModelItemRoles::NetworkData);

        plotNetwork(tNetwork);
    }
}

//=============================================================================================================

void NetworkTreeItem::plotNetwork(const Network& tNetworkData)
{
    //Draw network nodes and edges
    plotNodes(tNetworkData);
    plotEdges(tNetworkData);
}

//=============================================================================================================

void NetworkTreeItem::plotNodes(const Network& tNetworkData)
{
    if(tNetworkData.isEmpty()) {
        qDebug() << "NetworkTreeItem::plotNodes - Network data is empty. Returning.";
        return;
    }

    QList<NetworkNode::SPtr> lNetworkNodes = tNetworkData.getNodes();
    qint16 iMaxDegree = tNetworkData.getMinMaxThresholdedDegrees().second;

    VisualizationInfo visualizationInfo = tNetworkData.getVisualizationInfo();

    if(!m_pNodesEntity) {
        m_pNodesEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pNodes) {
        if(!m_pNodesGeometry) {
            m_pNodesGeometry = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
            m_pNodesGeometry->setRadius(0.6f);
        }

        m_pNodes = new GeometryMultiplier(m_pNodesGeometry);

        m_pNodesEntity->addComponent(m_pNodes);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(true);
        pMaterial->setAlpha(1.0);
        m_pNodesEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each sphere instance
    QVector<QMatrix4x4> vTransforms;
    QVector<QColor> vColorsNodes;
    QVector3D tempPos;
    qint16 iDegree = 0;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        iDegree = lNetworkNodes.at(i)->getThresholdedDegree();

        if(iDegree != 0) {
            tempPos = QVector3D(lNetworkNodes.at(i)->getVert()(0),
                                lNetworkNodes.at(i)->getVert()(1),
                                lNetworkNodes.at(i)->getVert()(2));

            //Set position and scale
            QMatrix4x4 tempTransform;
            tempTransform.translate(tempPos);
            tempTransform.scale(((float)iDegree/(float)iMaxDegree)*(0.005f-0.0006f)+0.0006f);

            vTransforms.push_back(tempTransform);

            if(visualizationInfo.sMethod == "Map") {
                // Normalize colors
                if(iMaxDegree != 0.0f) {
                    QColor color = ColorMap::valueToColor((float)iDegree/(float)iMaxDegree, visualizationInfo.sColormap);
                    color.setAlphaF(pow((float)iDegree/(float)iMaxDegree,4));
                    vColorsNodes.push_back(color);
                } else {
                    QColor color = ColorMap::valueToColor(0.0, visualizationInfo.sColormap);
                    //color.setAlphaF(0);
                    vColorsNodes.push_back(color);
                }
            } else {
                vColorsNodes.push_back(QColor(visualizationInfo.colEdges[0],
                                              visualizationInfo.colEdges[1],
                                              visualizationInfo.colEdges[2],
                                              visualizationInfo.colEdges[3]));
            }
        }
    }

    m_pNodes->setTransforms(vTransforms);
    m_pNodes->setColors(vColorsNodes);
}

//=============================================================================================================

void NetworkTreeItem::plotEdges(const Network &tNetworkData)
{
    if(tNetworkData.isEmpty()) {
        qDebug() << "NetworkTreeItem::plotEdges - Network data is empty. Returning.";
        return;
    }

    double dMaxWeight = tNetworkData.getMinMaxThresholdedWeights().second;
    double dMinWeight = tNetworkData.getMinMaxThresholdedWeights().first;

    QList<NetworkEdge::SPtr> lNetworkEdges = tNetworkData.getThresholdedEdges();
    QList<NetworkNode::SPtr> lNetworkNodes = tNetworkData.getNodes();

    VisualizationInfo visualizationInfo = tNetworkData.getVisualizationInfo();

    if(!m_pEdgeEntity) {
        m_pEdgeEntity = new QEntity(this);
    }

    //create geometry
    if(!m_pEdges) {
        if(!m_pEdgesGeometry) {
            m_pEdgesGeometry = QSharedPointer<Qt3DExtras::QCylinderGeometry>::create();
            m_pEdgesGeometry->setRadius(0.001f);
            m_pEdgesGeometry->setLength(1.0f);
        }

        m_pEdges = new GeometryMultiplier(m_pEdgesGeometry);

        m_pEdgeEntity->addComponent(m_pEdges);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(true);
        pMaterial->setAlpha(1.0f);
        m_pEdgeEntity->addComponent(pMaterial);
    }

    //Create transform matrix for each cylinder instance
    QVector<QMatrix4x4> vTransformsEdges;
    QVector<QColor> vColorsEdges;
    QVector3D startPos, endPos, edgePos, diff;
    double dWeight = 0.0;
    int iStartID, iEndID;

    for(int i = 0; i < lNetworkEdges.size(); ++i) {
        //Plot in edges
        NetworkEdge::SPtr pNetworkEdge = lNetworkEdges.at(i);

        if(pNetworkEdge->isActive()) {
            iStartID = pNetworkEdge->getStartNodeID();
            iEndID = pNetworkEdge->getEndNodeID();

            RowVectorXf vectorStart = lNetworkNodes.at(iStartID)->getVert();
            startPos = QVector3D(vectorStart(0),
                                 vectorStart(1),
                                 vectorStart(2));

            RowVectorXf vectorEnd = lNetworkNodes.at(iEndID)->getVert();
            endPos = QVector3D(vectorEnd(0),
                               vectorEnd(1),
                               vectorEnd(2));

            if(startPos != endPos) {
                dWeight = fabs(pNetworkEdge->getWeight());
                if(dWeight != 0.0) {
                    diff = endPos - startPos;
                    edgePos = endPos - diff/2;

                    QMatrix4x4 tempTransform;
                    tempTransform.translate(edgePos);
                    tempTransform.rotate(QQuaternion::rotationTo(QVector3D(0,1,0), diff.normalized()).normalized());
                    tempTransform.scale(fabs((dWeight-dMinWeight)/(dMaxWeight-dMinWeight)),diff.length(),fabs((dWeight-dMinWeight)/(dMaxWeight-dMinWeight)));
                    //tempTransform.scale(pow(fabs(dWeight/dMaxWeight),4)*4,diff.length(),pow(fabs(dWeight/dMaxWeight),4)*4);

                    vTransformsEdges.push_back(tempTransform);

                    // Colors
                    if(visualizationInfo.sMethod == "Map") {
                        // Normalize colors
                        if(dMaxWeight != 0.0f) {
                            QColor color = ColorMap::valueToColor(fabs(dWeight/dMaxWeight), visualizationInfo.sColormap);
                            color.setAlphaF(pow(fabs(dWeight/dMaxWeight),1.5));
                            //qDebug() << "fabs(dWeight/dMaxWeight)"<< fabs(dWeight/dMaxWeight);
                            //qDebug() << "color"<<color;
                            vColorsEdges.push_back(color);
                        } else {
                            QColor color = ColorMap::valueToColor(0.0, visualizationInfo.sColormap);
                            color.setAlphaF(0.0);
                            vColorsEdges.push_back(color);
                        }
                    } else {
                        vColorsEdges.push_back(QColor(visualizationInfo.colNodes[0],
                                                      visualizationInfo.colNodes[1],
                                                      visualizationInfo.colNodes[2],
                                                      visualizationInfo.colNodes[3]));
                    }
                }
            }
        }
    }

    m_pEdges->setTransforms(vTransformsEdges);
    m_pEdges->setColors(vColorsEdges);
}

