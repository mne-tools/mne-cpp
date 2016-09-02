//=============================================================================================================
/**
* @file     brainrtconnectivitydatatreeitem.cpp
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
* @brief    BrainRTConnectivityDataTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainrtconnectivitydatatreeitem.h"
#include "../../rt/rtSourceLoc/rtsourcelocdataworker.h"
#include "../common/metatreeitem.h"
#include "../../helpers/renderable3Dentity.h"

#include <fiff/fiff_types.h>

#include <mne/mne_sourceestimate.h>
#include <mne/mne_forwardsolution.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>

#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
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

BrainRTConnectivityDataTreeItem::BrainRTConnectivityDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Real time connectivity data");
}


//*************************************************************************************************************

BrainRTConnectivityDataTreeItem::~BrainRTConnectivityDataTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainRTConnectivityDataTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainRTConnectivityDataTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainRTConnectivityDataTreeItem::init(Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool BrainRTConnectivityDataTreeItem::addData(Network::SPtr pNetworkData)
{
    if(!m_bIsInit) {
        qDebug() << "BrainRTConnectivityDataTreeItem::updateData - BrainRTConnectivityDataTreeItem has not been initialized yet!";
        return false;
    }

    //Add data which is held by this BrainRTConnectivityDataTreeItem
    QVariant data;

    data.setValue(pNetworkData);
    this->setData(data, Data3DTreeModelItemRoles::NetworkData);

    MatrixXd matDist = pNetworkData->getConnectivityMatrix();
    data.setValue(matDist);
    this->setData(data, Data3DTreeModelItemRoles::NetworkDataMatrix);

    //Add surface meta information as item children
    QList<QStandardItem*> list;

    QVector3D vecEdgeTrehshold(0,0,0);
    MetaTreeItem* pItemNetworkThreshold = new MetaTreeItem(MetaTreeItemTypes::NetworkThreshold, "0.0,0.0,0.0");
    connect(pItemNetworkThreshold, &MetaTreeItem::networkThresholdChanged,
            this, &BrainRTConnectivityDataTreeItem::onNetworkThresholdChanged);
    list << pItemNetworkThreshold;
    list << new QStandardItem(pItemNetworkThreshold->toolTip());
    this->appendRow(list);
    data.setValue(vecEdgeTrehshold);
    pItemNetworkThreshold->setData(data, MetaTreeItemRoles::NetworkThreshold);

    list.clear();
    MetaTreeItem* pItemNetworkMatrix = new MetaTreeItem(MetaTreeItemTypes::NetworkMatrix, "Show network matrix");
    list << pItemNetworkMatrix;
    list << new QStandardItem(pItemNetworkMatrix->toolTip());
    this->appendRow(list);

    //Plot network
    plotNetwork(pNetworkData, vecEdgeTrehshold);

    return true;
}


//*************************************************************************************************************

void BrainRTConnectivityDataTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void BrainRTConnectivityDataTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_lNodes.size(); ++i) {
        m_lNodes.at(i)->setParent(state ? m_pRenderable3DEntity : Q_NULLPTR);
    }

    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
}


//*************************************************************************************************************

void BrainRTConnectivityDataTreeItem::onNetworkThresholdChanged(const QVector3D& vecThresholds)
{
    qDebug()<<"";
    Network::SPtr pNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network::SPtr>();

    plotNetwork(pNetwork, vecThresholds);
}


//*************************************************************************************************************

void BrainRTConnectivityDataTreeItem::plotNetwork(QSharedPointer<CONNECTIVITYLIB::Network> pNetworkData, const QVector3D& vecThreshold)
{
    //Prepare network visualization
    QMatrix4x4 m;
    Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
    m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
    transform->setMatrix(m);
    m_pRenderable3DEntity->addComponent(transform);

    QVector3D pos;
    QSharedPointer<Qt3DCore::QEntity> sourceSphereEntity;
    Qt3DExtras::QSphereMesh* sourceSphere;
    Qt3DExtras::QPhongMaterial* material;

    //Draw network nodes
    m_lNodes.clear();
    MatrixX3f tMatVert(pNetworkData->getNodes().size(), 3);
    MatrixX3f tMatNorm(pNetworkData->getNodes().size(), 3);
    tMatNorm.setZero();
    QList<NetworkNode::SPtr> lNetworkNodes = pNetworkData->getNodes();

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        pos.setX(lNetworkNodes.at(i)->getVert()(0));
        pos.setY(lNetworkNodes.at(i)->getVert()(1));
        pos.setZ(lNetworkNodes.at(i)->getVert()(2));

        tMatVert(i,0) = pos.x();
        tMatVert(i,1) = pos.y();
        tMatVert(i,2) = pos.z();

        sourceSphereEntity = QSharedPointer<Qt3DCore::QEntity>(new Qt3DCore::QEntity(m_pRenderable3DEntity));

        sourceSphere = new Qt3DExtras::QSphereMesh();
        sourceSphere->setRadius(0.001f);
        sourceSphereEntity->addComponent(sourceSphere);

        Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
        QMatrix4x4 m;
        m.translate(pos);
        transform->setMatrix(m);
        sourceSphereEntity->addComponent(transform);

        material = new Qt3DExtras::QPhongMaterial();
        material->setAmbient(Qt::yellow);
        sourceSphereEntity->addComponent(material);
        m_lNodes << sourceSphereEntity;
    }

    //Generate connection indices for Qt3D buffer
    MatrixX3i tMatTris;
    int count = 0;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        //Plot in edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesIn().size(); ++j) {
            if(lNetworkNodes.at(i)->getEdgesIn().at(j)->getWeight() >= vecThreshold.x()) {
                tMatTris.conservativeResize(count+1,3);
                tMatTris(count,0) = lNetworkNodes.at(i)->getEdgesIn().at(j)->getStartNode()->getId();
                tMatTris(count,1) = lNetworkNodes.at(i)->getEdgesIn().at(j)->getEndNode()->getId();
                tMatTris(count,2) = lNetworkNodes.at(i)->getEdgesIn().at(j)->getStartNode()->getId();
                ++count;
            }
        }

        //Plot out edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesOut().size(); ++j) {
            if(lNetworkNodes.at(i)->getEdgesOut().at(j)->getWeight() >= vecThreshold.x()) {
                tMatTris.conservativeResize(count+1,3);
                tMatTris(count,0) = lNetworkNodes.at(i)->getEdgesOut().at(j)->getStartNode()->getId();
                tMatTris(count,1) = lNetworkNodes.at(i)->getEdgesOut().at(j)->getEndNode()->getId();
                tMatTris(count,2) = lNetworkNodes.at(i)->getEdgesOut().at(j)->getStartNode()->getId();
                ++count;
            }
        }
    }

    //Generate line primitive based network
    m_pRenderable3DEntity->setMeshData(tMatVert, tMatNorm, tMatTris, QByteArray(), Qt3DRender::QGeometryRenderer::Lines);
}


