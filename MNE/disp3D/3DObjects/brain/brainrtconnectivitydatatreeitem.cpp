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

#include "fiff/fiff_types.h"

#include "mne/mne_sourceestimate.h"
#include "mne/mne_forwardsolution.h"


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


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainRTConnectivityDataTreeItem::BrainRTConnectivityDataTreeItem(int iType, const QString &text)
: AbstractTreeItem(iType, text)
, m_bIsInit(false)
{
    this->setEditable(false);
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

bool BrainRTConnectivityDataTreeItem::init(const MNEForwardSolution& tForwardSolution, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Set data based on clusterd or full source space
    bool isClustered = tForwardSolution.isClustered();

    //Add source vertices to data
    QVariant data;
    data.setValue(tForwardSolution.source_rr);
    this->setData(data, Data3DTreeModelItemRoles::SourceVertices);

    //Add meta information as item children
    QString sIsClustered = isClustered ? "Clustered" : "Full";
    MetaTreeItem* pItemSourceSpaceType = new MetaTreeItem(MetaTreeItemTypes::RTDataSourceSpaceType, sIsClustered);
    pItemSourceSpaceType->setEditable(false);
    *this << pItemSourceSpaceType;
    data.setValue(sIsClustered);
    pItemSourceSpaceType->setData(data, MetaTreeItemRoles::RTDataSourceSpaceType);

    m_bIsInit = true;

    return true;
}


//*************************************************************************************************************

bool BrainRTConnectivityDataTreeItem::addData(const MatrixXd& matNewConnection)
{
    Q_UNUSED(matNewConnection)
    if(!m_bIsInit) {
        qDebug() << "BrainRTConnectivityDataTreeItem::updateData - Rt Item has not been initialized yet!";
        return false;
    }

    MatrixX3f matSourceVert = this->data(Data3DTreeModelItemRoles::SourceVertices).value<MatrixX3f>();
    MatrixX3f matSourceNorm = MatrixX3f::Zero(matSourceVert.rows(),3);

    if(matSourceVert.rows() != matNewConnection.rows())
    {
        qDebug() << "BrainRTConnectivityDataTreeItem::updateData - Number of network nodes and sources do not match!";
        return false;
    }

    //Visualize network
    double dEdgeTrehshold = 500;
    QMatrix4x4 m;
    Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
    m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
    transform->setMatrix(m);
    m_pRenderable3DEntity->addComponent(transform);

    RowVector3f sourcePos;
    QVector3D pos;
    Qt3DCore::QEntity* sourceSphereEntity;
    Qt3DExtras::QSphereMesh* sourceSphere;
    Qt3DExtras::QPhongMaterial* material;

    //Draw network nodes and generate connection indices for Qt3D buffer
    for(int i = 0; i < matNewConnection.rows(); ++i)
    {
        sourcePos = matSourceVert.row(i);
        pos.setX(sourcePos(0));
        pos.setY(sourcePos(1));
        pos.setZ(sourcePos(2));

        sourceSphereEntity = new Qt3DCore::QEntity();

        sourceSphere = new Qt3DExtras::QSphereMesh();
        sourceSphere->setRadius(0.001f);
        sourceSphereEntity->addComponent(sourceSphere);

        transform = new Qt3DCore::QTransform();
        QMatrix4x4 m;
        m.translate(pos);
        transform->setMatrix(m);
        sourceSphereEntity->addComponent(transform);

        material = new Qt3DExtras::QPhongMaterial();
        material->setAmbient(Qt::yellow);
        sourceSphereEntity->addComponent(material);

        sourceSphereEntity->setParent(m_pRenderable3DEntity);
    }

    //m_pRenderable3DEntity->setMeshData(matSourceVert, matSourceNorm, tSurface.tris(), arrayCurvatureColor, Qt3DRender::QGeometryRenderer::Lines);

    return true;
}

