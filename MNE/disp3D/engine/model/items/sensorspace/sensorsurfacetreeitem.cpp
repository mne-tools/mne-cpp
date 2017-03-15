//=============================================================================================================
/**
* @file     sensorsurfacetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     March, 2017
*
* @section  LICENSE
*
* Copyright (C) 2017, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    SensorSurfaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "sensorsurfacetreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/pervertexphongalphamaterial.h"

#include <mne/mne_bem.h>
#include <fiff/fiff_constants.h>


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

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FSLIB;
using namespace MNELIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

SensorSurfaceTreeItem::SensorSurfaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    initItem();
}


//*************************************************************************************************************

SensorSurfaceTreeItem::~SensorSurfaceTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    m_pRenderable3DEntity->deleteLater();
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Sensor surface item");

    //Add surface meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    float fAlpha = 0.6;
    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::AlphaValue, QString::number(fAlpha));
    connect(itemAlpha, &MetaTreeItem::alphaChanged,
            this, &SensorSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list << itemAlpha;
    list << new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(fAlpha);
    itemAlpha->setData(data, MetaTreeItemRoles::AlphaValue);

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::Color, "Surface color");
    connect(pItemSurfCol, &MetaTreeItem::colorChanged,
            this, &SensorSurfaceTreeItem::onSurfaceColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, MetaTreeItemRoles::Color);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    MetaTreeItem *itemXTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateX, QString::number(0));
    itemXTrans->setEditable(true);
    connect(itemXTrans, &MetaTreeItem::surfaceTranslationXChanged,
            this, &SensorSurfaceTreeItem::onSurfaceTranslationXChanged);
    list.clear();
    list << itemXTrans;
    list << new QStandardItem(itemXTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemYTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateY, QString::number(0));
    itemYTrans->setEditable(true);
    connect(itemYTrans, &MetaTreeItem::surfaceTranslationYChanged,
            this, &SensorSurfaceTreeItem::onSurfaceTranslationYChanged);
    list.clear();
    list << itemYTrans;
    list << new QStandardItem(itemYTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemZTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateZ, QString::number(0));
    itemZTrans->setEditable(true);
    connect(itemZTrans, &MetaTreeItem::surfaceTranslationZChanged,
            this, &SensorSurfaceTreeItem::onSurfaceTranslationZChanged);
    list.clear();
    list << itemZTrans;
    list << new QStandardItem(itemZTrans->toolTip());
    this->appendRow(list);
}


//*************************************************************************************************************

QVariant SensorSurfaceTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  SensorSurfaceTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
        case Data3DTreeModelItemRoles::SurfaceCurrentColorVert:
            m_pRenderable3DEntity->getCustomMesh()->setColor(value.value<MatrixX3f>());
            break;
        default: // do nothing;
            break;
    }
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::addData(const MNEBemSurface& tSensorSurface, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pRenderable3DEntity->setParent(parent);

    //Create color from curvature information with default gyri and sulcus colors
    MatrixX3f matVertColor = createVertColor(tSensorSurface.rr);

    //Set renderable 3D entity mesh and color data
    m_pRenderable3DEntity->getCustomMesh()->setMeshData(tSensorSurface.rr, tSensorSurface.nn, tSensorSurface.tris, matVertColor, Qt3DRender::QGeometryRenderer::Triangles);

    //Set shaders
    PerVertexPhongAlphaMaterial* pPerVertexPhongAlphaMaterial = new PerVertexPhongAlphaMaterial();
    m_pRenderable3DEntity->addComponent(pPerVertexPhongAlphaMaterial);

    //Add data which is held by this SensorSurfaceTreeItem
    QVariant data;

    data.setValue(matVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

    data.setValue(tSensorSurface.rr);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceVert);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::setVisible(bool state)
{
    m_pRenderable3DEntity->setEnabled(state);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onSurfaceAlphaChanged(float fAlpha)
{
    m_pRenderable3DEntity->setMaterialParameter(fAlpha, "alpha");
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onSurfaceColorChanged(const QColor& color)
{
    QVariant data;
    MatrixX3f matNewVertColor = createVertColor(this->data(Data3DTreeModelItemRoles::SurfaceVert).value<MatrixX3f>(), color);

    data.setValue(matNewVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState==Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onSurfaceTranslationXChanged(float fTransX)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setX(fTransX);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onSurfaceTranslationYChanged(float fTransY)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setY(fTransY);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void SensorSurfaceTreeItem::onSurfaceTranslationZChanged(float fTransZ)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setZ(fTransZ);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

MatrixX3f SensorSurfaceTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color) const
{
    MatrixX3f matCurvatureColor(vertices.rows(),3);

    for(int i = 0; i < matCurvatureColor.rows(); ++i) {
        matCurvatureColor(i,0) = color.redF();
        matCurvatureColor(i,1) = color.greenF();
        matCurvatureColor(i,2) = color.blueF();
    }

    return matCurvatureColor;
}
