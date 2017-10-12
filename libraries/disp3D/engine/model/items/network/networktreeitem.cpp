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
#include "../../workers/rtSourceLoc/rtsourcelocdataworker.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/networkmaterial.h"
#include "../../3dhelpers/custommesh.h"
#include "../../3dhelpers/geometrymultiplier.h"
#include "../../materials/geometrymultipliermaterial.h"

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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

NetworkTreeItem::NetworkTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString &text)
: AbstractMeshTreeItem(p3DEntityParent, iType, text)
, m_bNodesPlotted(false)
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
        m_pItemNetworkThreshold = new MetaTreeItem(MetaTreeItemTypes::NetworkThreshold,
                                                    QString("%1,%2,%3").arg(vecEdgeTrehshold.x()).arg(vecEdgeTrehshold.y()).arg(vecEdgeTrehshold.z()));
    }

    list << m_pItemNetworkThreshold;
    list << new QStandardItem(m_pItemNetworkThreshold->toolTip());
    this->appendRow(list);
    data.setValue(vecEdgeTrehshold);
    m_pItemNetworkThreshold->setData(data, MetaTreeItemRoles::NetworkThreshold);
    connect(m_pItemNetworkThreshold.data(), &MetaTreeItem::dataChanged,
            this, &NetworkTreeItem::onNetworkThresholdChanged);

    list.clear();
    MetaTreeItem* pItemNetworkMatrix = new MetaTreeItem(MetaTreeItemTypes::NetworkMatrix, "Show network matrix");
    list << pItemNetworkMatrix;
    list << new QStandardItem(pItemNetworkMatrix->toolTip());
    this->appendRow(list);

    //Set shaders
    this->removeComponent(m_pMaterial);
    this->removeComponent(m_pTessMaterial);
    this->removeComponent(m_pNormalMaterial);

    NetworkMaterial* pNetworkMaterial = new NetworkMaterial();
    this->addComponent(pNetworkMaterial);
}


//*************************************************************************************************************

void NetworkTreeItem::addData(const Network& tNetworkData)
{
    //Add data which is held by this NetworkTreeItem
    QVariant data;

    data.setValue(tNetworkData);
    this->setData(data, Data3DTreeModelItemRoles::NetworkData);

    MatrixXd matDist = tNetworkData.getConnectivityMatrix();
    data.setValue(matDist);
    this->setData(data, Data3DTreeModelItemRoles::NetworkDataMatrix);

    //Plot network
    if(m_pItemNetworkThreshold) {
        plotNetwork(tNetworkData,
                    m_pItemNetworkThreshold->data(MetaTreeItemRoles::NetworkThreshold).value<QVector3D>());
    }
}


//*************************************************************************************************************

void NetworkTreeItem::onNetworkThresholdChanged(const QVariant& vecThresholds)
{
    if(vecThresholds.canConvert<QVector3D>()) {
        Network tNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network>();

        plotNetwork(tNetwork, vecThresholds.value<QVector3D>());
    }
}


//*************************************************************************************************************

void NetworkTreeItem::plotNetwork(const Network& tNetworkData, const QVector3D& vecThreshold)
{
    //Create network vertices and normals
    QList<NetworkNode::SPtr> lNetworkNodes = tNetworkData.getNodes();

    MatrixX3f tMatVert(lNetworkNodes.size(), 3);

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        tMatVert(i,0) = lNetworkNodes.at(i)->getVert()(0);
        tMatVert(i,1) = lNetworkNodes.at(i)->getVert()(1);
        tMatVert(i,2) = lNetworkNodes.at(i)->getVert()(2);
    }

    MatrixX3f tMatNorm(lNetworkNodes.size(), 3);
    tMatNorm.setZero();

    //Draw network nodes
    //TODO: Dirty hack using m_bNodesPlotted flag to get rid of memory leakage problem when putting parent to the nodes entities. Internal Qt3D problem?
    if(!m_bNodesPlotted) {

        Renderable3DEntity* pSourceSphereEntity = new Renderable3DEntity(this);
        //create geometry
        QSharedPointer<Qt3DExtras::QSphereGeometry> pSourceSphereGeometry = QSharedPointer<Qt3DExtras::QSphereGeometry>::create();
        pSourceSphereGeometry->setRadius(0.001f);
        //create instanced renderer
        GeometryMultiplier *pSphereMesh = new GeometryMultiplier(pSourceSphereGeometry);

        //Create transform matrix for each sphere instance
        QVector<QMatrix4x4> vTransforms;
        vTransforms.reserve(tMatVert.rows());
        QVector3D tempPos;

        for(int i = 0; i < tMatVert.rows(); ++i) {
            QMatrix4x4 tempTransform;

            tempPos.setX(tMatVert(i, 0));
            tempPos.setY(tMatVert(i, 1));
            tempPos.setZ(tMatVert(i, 2));
            //Set position
            tempTransform.translate(tempPos);
            vTransforms.push_back(tempTransform);
        }

        //Set instance Transform
        pSphereMesh->setTransforms(vTransforms);

        pSourceSphereEntity->addComponent(pSphereMesh);

        //Add material
        GeometryMultiplierMaterial* pMaterial = new GeometryMultiplierMaterial(true);
        pMaterial->setAmbient(Qt::blue);
        pMaterial->setAlpha(1.0f);
        pSourceSphereEntity->addComponent(pMaterial);

        m_bNodesPlotted = true;
    }

    //Generate connection indices for Qt3D buffer
    MatrixXi tMatLines;
    int count = 0;
    int start, end;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        //Plot in edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesIn().size(); ++j) {
            start = lNetworkNodes.at(i)->getEdgesIn().at(j)->getStartNode()->getId();
            end = lNetworkNodes.at(i)->getEdgesIn().at(j)->getEndNode()->getId();

            if(std::fabs(lNetworkNodes.at(i)->getEdgesIn().at(j)->getWeight()) >= vecThreshold.x() &&
                    start != end) {
                tMatLines.conservativeResize(count+1,2);
                tMatLines(count,0) = start;
                tMatLines(count,1) = end;
                ++count;
            }
        }

        //Plot out edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesOut().size(); ++j) {
            start = lNetworkNodes.at(i)->getEdgesOut().at(j)->getStartNode()->getId();
            end = lNetworkNodes.at(i)->getEdgesOut().at(j)->getEndNode()->getId();

            if(std::fabs(lNetworkNodes.at(i)->getEdgesOut().at(j)->getWeight()) >= vecThreshold.x() &&
                    start != end) {
                tMatLines.conservativeResize(count+1,2);
                tMatLines(count,0) = start;
                tMatLines(count,1) = end;
                ++count;
            }
        }
    }

    //Generate colors for Qt3D buffer
    MatrixX3f matLineColor(tMatVert.rows(),3);

    for(int i = 0; i < matLineColor.rows(); ++i) {
        matLineColor(i,0) = 0.0f;
        matLineColor(i,1) = 0.0f;
        matLineColor(i,2) = 1.0f;
    }

    m_pCustomMesh->setMeshData(tMatVert,
                                tMatNorm,
                                tMatLines,
                                matLineColor,
                                Qt3DRender::QGeometryRenderer::Lines);
}


