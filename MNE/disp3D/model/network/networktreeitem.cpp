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
#include "../workers/rtSourceLoc/rtsourcelocdataworker.h"
#include "../common/metatreeitem.h"
#include "../common/renderable3Dentity.h"

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
#include <QUrl>


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

NetworkTreeItem::NetworkTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
, m_bNodesPlotted(false)
, m_pItemNetworkThreshold(new MetaTreeItem())
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Network/Connectivity data");
}


//*************************************************************************************************************

NetworkTreeItem::~NetworkTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    //Cannot delete m_pParentEntity since we do not know who else holds it, that is why we use a QPointer for m_pParentEntity.
    for(int i = 0; i < m_lNodes.size(); ++i) {
        m_lNodes.at(i)->deleteLater();
    }

    if(!m_pRenderable3DEntity.isNull()) {
        m_pRenderable3DEntity->deleteLater();
    }
}


//*************************************************************************************************************

QVariant NetworkTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  NetworkTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

bool NetworkTreeItem::init(Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Set shaders
    m_pRenderable3DEntity->setShader(QUrl(QStringLiteral("qrc:/model/common/shaders/gl3/networkEdge.vert")));
    m_pRenderable3DEntity->setShader(QUrl(QStringLiteral("qrc:/model/common/shaders/gl3/networkEdge.frag")));

    //Add meta information as item children
    QList<QStandardItem*> list;

    QVariant data;

    QVector3D vecEdgeTrehshold(0,5,10);
    m_pItemNetworkThreshold = new MetaTreeItem(MetaTreeItemTypes::NetworkThreshold, "0.0,5.0,10.0");
    list << m_pItemNetworkThreshold;
    list << new QStandardItem(m_pItemNetworkThreshold->toolTip());
    this->appendRow(list);
    data.setValue(vecEdgeTrehshold);
    m_pItemNetworkThreshold->setData(data, MetaTreeItemRoles::NetworkThreshold);
    connect(m_pItemNetworkThreshold, &MetaTreeItem::networkThresholdChanged,
            this, &NetworkTreeItem::onNetworkThresholdChanged);

    list.clear();
    MetaTreeItem* pItemNetworkMatrix = new MetaTreeItem(MetaTreeItemTypes::NetworkMatrix, "Show network matrix");
    list << pItemNetworkMatrix;
    list << new QStandardItem(pItemNetworkMatrix->toolTip());
    this->appendRow(list);

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool NetworkTreeItem::addData(Network::SPtr pNetworkData)
{
    if(!m_bIsInit) {
        qDebug() << "NetworkTreeItem::updateData - NetworkTreeItem has not been initialized yet!";
        return false;
    }

    //Add data which is held by this NetworkTreeItem
    QVariant data;

    data.setValue(pNetworkData);
    this->setData(data, Data3DTreeModelItemRoles::NetworkData);

    MatrixXd matDist = pNetworkData->getConnectivityMatrix();
    data.setValue(matDist);
    this->setData(data, Data3DTreeModelItemRoles::NetworkDataMatrix);

    //Plot network
    QVector3D vecThreshold = m_pItemNetworkThreshold->data(MetaTreeItemRoles::NetworkThreshold).value<QVector3D>();
    plotNetwork(pNetworkData, vecThreshold);

    return true;
}


//*************************************************************************************************************

void NetworkTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void NetworkTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_lNodes.size(); ++i) {
        m_lNodes.at(i)->setParent(state ? m_pRenderable3DEntity : Q_NULLPTR);
    }

    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
}


//*************************************************************************************************************

void NetworkTreeItem::onNetworkThresholdChanged(const QVector3D& vecThresholds)
{
    Network::SPtr pNetwork = this->data(Data3DTreeModelItemRoles::NetworkData).value<Network::SPtr>();

    plotNetwork(pNetwork, vecThresholds);
}


//*************************************************************************************************************

void NetworkTreeItem::plotNetwork(QSharedPointer<CONNECTIVITYLIB::Network> pNetworkData, const QVector3D& vecThreshold)
{
//    // Delete all old renderable children
//    Renderable3DEntity* pParentTemp = new Renderable3DEntity();
//    QList<QObject*> list = m_pRenderable3DEntity->children();
//    QMutableListIterator<QObject*> i(list);
//    int counter = 0;

//    while (i.hasNext()) {
//        if(Renderable3DEntity* entity = dynamic_cast<Renderable3DEntity*>(i.next())) {
//            entity->setParent(pParentTemp);
//            delete entity;
//            i.remove();
//            counter++;
//        }
//    }

//    delete pParentTemp;

//    qDebug() << "Deleted children from qt3d entity:" << counter;

//    //Delete all nodes
//    QMutableListIterator<QPointer<Renderable3DEntity> > i(m_lNodes);

//    while(i.hasNext()) {
//        delete i.next();
//        i.remove();
//    }

//    m_lNodes.clear();

    //Create network vertices and normals
    QList<NetworkNode::SPtr> lNetworkNodes = pNetworkData->getNodes();

    MatrixX3f tMatVert(pNetworkData->getNodes().size(), 3);

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        tMatVert(i,0) = lNetworkNodes.at(i)->getVert()(0);
        tMatVert(i,1) = lNetworkNodes.at(i)->getVert()(1);
        tMatVert(i,2) = lNetworkNodes.at(i)->getVert()(2);
    }

    MatrixX3f tMatNorm(pNetworkData->getNodes().size(), 3);
    tMatNorm.setZero();

    //Draw network nodes
    //TODO: Dirty hack using m_bNodesPlotted flag to get rid of memory leakage problem when putting parent to the nodes entities. Internal Qt3D problem?
    if(!m_bNodesPlotted) {
        QVector3D pos;

        for(int i = 0; i < lNetworkNodes.size(); ++i) {
            pos.setX(lNetworkNodes.at(i)->getVert()(0));
            pos.setY(lNetworkNodes.at(i)->getVert()(1));
            pos.setZ(lNetworkNodes.at(i)->getVert()(2));

            Renderable3DEntity* sourceSphereEntity = new Renderable3DEntity(m_pRenderable3DEntity);

            Qt3DExtras::QSphereMesh* sourceSphere = new Qt3DExtras::QSphereMesh();
            sourceSphere->setRadius(0.001f);
            sourceSphereEntity->addComponent(sourceSphere);

            Qt3DCore::QTransform* transform = new Qt3DCore::QTransform();
            QMatrix4x4 m;
            m.translate(pos);
            transform->setMatrix(m);
            sourceSphereEntity->addComponent(transform);

            Qt3DExtras::QPhongMaterial* material = new Qt3DExtras::QPhongMaterial();
            material->setAmbient(Qt::blue);
            sourceSphereEntity->addComponent(material);

            m_lNodes.append(sourceSphereEntity);
        }

        m_bNodesPlotted = true;
    }

    //Generate connection indices for Qt3D buffer
    MatrixXi tMatLines;
    int count = 0;

    for(int i = 0; i < lNetworkNodes.size(); ++i) {
        //Plot in edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesIn().size(); ++j) {
            if(lNetworkNodes.at(i)->getEdgesIn().at(j)->getWeight() >= vecThreshold.x()) {
                tMatLines.conservativeResize(count+1,2);
                tMatLines(count,0) = lNetworkNodes.at(i)->getEdgesIn().at(j)->getStartNode()->getId();
                tMatLines(count,1) = lNetworkNodes.at(i)->getEdgesIn().at(j)->getEndNode()->getId();
                ++count;
            }
        }

        //Plot out edges
        for(int j = 0; j < lNetworkNodes.at(i)->getEdgesOut().size(); ++j) {
            if(lNetworkNodes.at(i)->getEdgesOut().at(j)->getWeight() >= vecThreshold.x()) {
                tMatLines.conservativeResize(count+1,2);
                tMatLines(count,0) = lNetworkNodes.at(i)->getEdgesOut().at(j)->getStartNode()->getId();
                tMatLines(count,1) = lNetworkNodes.at(i)->getEdgesOut().at(j)->getEndNode()->getId();
                ++count;
            }
        }
    }

    //Generate connection indices and colors for Qt3D buffer
    QByteArray arrayLineColor;
    arrayLineColor.resize(tMatVert.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(arrayLineColor.data());
    int idxColor = 0;

    for(int i = 0; i < tMatVert.rows(); ++i) {
        rawColorArray[idxColor] = 0.0f;
        idxColor++;
        rawColorArray[idxColor] = 0.0f;
        idxColor++;
        rawColorArray[idxColor] = 1.0f;
        idxColor++;
    }

    //Generate line primitive based network
    m_pRenderable3DEntity->setMeshData(tMatVert, tMatNorm, tMatLines, arrayLineColor, Qt3DRender::QGeometryRenderer::Lines);
}


