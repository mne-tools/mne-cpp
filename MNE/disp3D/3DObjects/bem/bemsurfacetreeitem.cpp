//=============================================================================================================
/**
* @file     bemsurfacetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     May, 2016
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
* @brief    BemSurfaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "bemsurfacetreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FSLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BemSurfaceTreeItem::BemSurfaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("BEM surface");
}


//*************************************************************************************************************

BemSurfaceTreeItem::~BemSurfaceTreeItem()
{
}


//*************************************************************************************************************

QVariant BemSurfaceTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BemSurfaceTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
    case Data3DTreeModelItemRoles::SurfaceCurrentColorVert:
        m_pRenderable3DEntity->setVertColor(value.value<QByteArray>());
        break;
    }
}


//*************************************************************************************************************

bool BemSurfaceTreeItem::addData(const MNEBemSurface& tBemSurface, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    QMatrix4x4 m;
    Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
    m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
    transform->setMatrix(m);
    m_pRenderable3DEntity->addComponent(transform);

    //Create color from curvature information with default gyri and sulcus colors
    QByteArray arrayVertColor = createVertColor(tBemSurface.rr);

    //Set renderable 3D entity mesh and color data
    Vector3f offset(3);
    offset << 0.0, 0.0, 0.0;

    m_pRenderable3DEntity->setMeshData(tBemSurface.rr, tBemSurface.nn, tBemSurface.tris, offset, arrayVertColor);

    //Find out BEM layer type and change items name
    this->setText(MNEBemSurface::id_name(tBemSurface.id));

    //Add data which is held by this BemSurfaceTreeItem
    QVariant data;

    data.setValue(arrayVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

    data.setValue(tBemSurface.rr);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceVert);

    data.setValue(tBemSurface.tris);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceTris);

    data.setValue(tBemSurface.nn);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceNorm);

    data.setValue(offset);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceOffset);

    data.setValue(m_pRenderable3DEntity);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceRenderable3DEntity);

    //Add surface meta information as item children
    QList<QStandardItem*> list;

    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::SurfaceAlpha, "0.5");
    connect(itemAlpha, &MetaTreeItem::surfaceAlphaChanged,
            this, &BemSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list<<itemAlpha;
    list<<new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(0.5);
    itemAlpha->setData(data, MetaTreeItemRoles::SurfaceAlpha);

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::SurfaceColor, "Surface color");
    connect(pItemSurfCol, &MetaTreeItem::surfaceColorChanged,
            this, &BemSurfaceTreeItem::onSurfaceColorChanged);
    list.clear();
    list<<pItemSurfCol;
    list<<new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, MetaTreeItemRoles::SurfaceColor);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    return true;
}


//*************************************************************************************************************

void BemSurfaceTreeItem::setVisible(bool state)
{
    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onSurfaceAlphaChanged(float fAlpha)
{
    m_pRenderable3DEntity->setAlpha(fAlpha);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onSurfaceColorChanged(const QColor& color)
{
    QVariant data;
    QByteArray arrayNewVertColor = createVertColor(this->data(Data3DTreeModelItemRoles::SurfaceVert).value<MatrixX3f>(), color);

    data.setValue(arrayNewVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState==Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

QByteArray BemSurfaceTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color) const
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
