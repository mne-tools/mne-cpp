//=============================================================================================================
/**
* @file     brainsourcespacetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     January, 2016
*
* @section  LICENSE
*
* Copyright (C) 2015, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    BrainSourceSpaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsourcespacetreeitem.h"


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

BrainSourceSpaceTreeItem::BrainSourceSpaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setToolTip("Source space");
}


//*************************************************************************************************************

BrainSourceSpaceTreeItem::~BrainSourceSpaceTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSourceSpaceTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainSourceSpaceTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
    case BrainSourceSpaceTreeItemRoles::SurfaceCurrentColorVert:
        m_pRenderable3DEntity->setVertColor(value.value<QByteArray>());
        break;
    }
}


//*************************************************************************************************************

bool BrainSourceSpaceTreeItem::addData(const MNEHemisphere& tHemisphere, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    QMatrix4x4 m;
    Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
    m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
    transform->setMatrix(m);
    m_pRenderable3DEntity->addComponent(transform);

    //Create sources as small 3D spheres
    RowVector3f sourcePos;
    QVector3D pos;
    Qt3DCore::QEntity* sourceSphereEntity;
    Qt3DRender::QSphereMesh* sourceSphere;
    Qt3DRender::QPhongMaterial* material;

    if(tHemisphere.isClustered()) {
        for(int i = 0; i < tHemisphere.cluster_info.centroidVertno.size(); i++) {
            sourcePos = tHemisphere.rr.row(tHemisphere.cluster_info.centroidVertno.at(i));
            pos.setX(sourcePos(0));
            pos.setY(sourcePos(1));
            pos.setZ(sourcePos(2));

            sourceSphereEntity = new Qt3DCore::QEntity();

            sourceSphere = new Qt3DRender::QSphereMesh();
            sourceSphere->setRadius(0.001f);
            sourceSphereEntity->addComponent(sourceSphere);

            transform = new Qt3DCore::QTransform();
            QMatrix4x4 m;
            m.translate(pos);
            transform->setMatrix(m);
            sourceSphereEntity->addComponent(transform);

            material = new Qt3DRender::QPhongMaterial();
            material->setAmbient(QColor(255,0,0));
            sourceSphereEntity->addComponent(material);

            sourceSphereEntity->setParent(m_pRenderable3DEntity);
        }
    } else {
        for(int i = 0; i < tHemisphere.vertno.rows(); i++) {
            sourcePos = tHemisphere.rr.row(tHemisphere.vertno(i));
            pos.setX(sourcePos(0));
            pos.setY(sourcePos(1));
            pos.setZ(sourcePos(2));

            sourceSphereEntity = new Qt3DCore::QEntity();

            sourceSphere = new Qt3DRender::QSphereMesh();
            sourceSphere->setRadius(0.001f);
            sourceSphereEntity->addComponent(sourceSphere);

            transform = new Qt3DCore::QTransform();
            QMatrix4x4 m;
            m.translate(pos);
            transform->setMatrix(m);
            sourceSphereEntity->addComponent(transform);

            material = new Qt3DRender::QPhongMaterial();
            material->setAmbient(QColor(255,0,0));
            sourceSphereEntity->addComponent(material);

            sourceSphereEntity->setParent(m_pRenderable3DEntity);
        }
    }

    //Create color from curvature information with default gyri and sulcus colors
    QByteArray arrayVertColor = createVertColor(tHemisphere.rr);

    //Set renderable 3D entity mesh and color data
    Vector3f offset(3);
    offset<<0.0, 0.0, 0.0;

    m_pRenderable3DEntity->setMeshData(tHemisphere.rr, tHemisphere.nn, tHemisphere.tris, offset, arrayVertColor);

    //Add data which is held by this BrainSourceSpaceTreeItem
    QVariant data;

    data.setValue(arrayVertColor);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceCurrentColorVert);

    data.setValue(tHemisphere.rr);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceVert);

    data.setValue(tHemisphere.tris);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceTris);

    data.setValue(tHemisphere.nn);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceNorm);

    data.setValue(offset);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceOffset);

    data.setValue(m_pRenderable3DEntity);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceRenderable3DEntity);

    //Add surface meta information as item children
    BrainTreeMetaItem* pItemSurfCol = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceColorItem, "Surface color");
    connect(pItemSurfCol, &BrainTreeMetaItem::surfaceColorChanged,
            this, &BrainSourceSpaceTreeItem::onSurfaceColorChanged);
    *this<<pItemSurfCol;
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, BrainTreeMetaItemRoles::SurfaceColor);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    return true;
}


//*************************************************************************************************************

void BrainSourceSpaceTreeItem::onSurfaceColorChanged(const QColor& color)
{
    QVariant data;
    QByteArray arrayNewVertColor = createVertColor(this->data(BrainSourceSpaceTreeItemRoles::SurfaceVert).value<MatrixX3f>(), color);

    data.setValue(arrayNewVertColor);
    this->setData(data, BrainSourceSpaceTreeItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

QByteArray BrainSourceSpaceTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color)
{
    QByteArray arrayCurvatureColor;
    arrayCurvatureColor.resize(vertices.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(arrayCurvatureColor.data());
    int idxColor = 0;

    for(int i = 0; i<vertices.rows(); i++) {
        rawColorArray[idxColor++] = color.redF();
        rawColorArray[idxColor++] = color.greenF();
        rawColorArray[idxColor++] = color.blueF();
    }

    return arrayCurvatureColor;
}
