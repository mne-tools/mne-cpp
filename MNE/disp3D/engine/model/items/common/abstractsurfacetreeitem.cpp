//=============================================================================================================
/**
* @file     abstractsurfacetreeitem.cpp
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
* @brief    AbstractSurfaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstractsurfacetreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/pervertexphongalphamaterial.h"
#include "../../materials/pervertextessphongalphamaterial.h"
#include "../../materials/shownormalsmaterial.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include <Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;

//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractSurfaceTreeItem::AbstractSurfaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
, m_pMaterial(new PerVertexPhongAlphaMaterial())
, m_pTessMaterial(new PerVertexTessPhongAlphaMaterial())
, m_pNormalMaterial(new ShowNormalsMaterial())
{
    initItem();
}


//*************************************************************************************************************

AbstractSurfaceTreeItem::~AbstractSurfaceTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    if(m_pRenderable3DEntity) {
        m_pRenderable3DEntity->deleteLater();
    }
}


//*************************************************************************************************************

QPointer<Renderable3DEntity> AbstractSurfaceTreeItem::getRenderableEntity()
{
    return m_pRenderable3DEntity;
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Abstract Surface item");

    //Add surface meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    float fAlpha = 0.35f;
    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::AlphaValue, QString("%1").arg(fAlpha));
    connect(itemAlpha, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list << itemAlpha;
    list << new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(fAlpha);
    itemAlpha->setData(data, MetaTreeItemRoles::AlphaValue);

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::Color, "Surface color");
    connect(pItemSurfCol, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, MetaTreeItemRoles::Color);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    QString surfaceType("Phong Alpha");
    MetaTreeItem* pItemMaterialType = new MetaTreeItem(MetaTreeItemTypes::MaterialType, surfaceType);
    connect(pItemMaterialType, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceMaterialChanged);
    list.clear();
    list << pItemMaterialType;
    list << new QStandardItem(pItemMaterialType->toolTip());
    this->appendRow(list);
    data.setValue(QString(surfaceType));
    pItemMaterialType->setData(data, MetaTreeItemRoles::SurfaceMaterial);
    pItemMaterialType->setData(data, Qt::DecorationRole);

    float fTessInner = 1.0;
    MetaTreeItem *itemTessInner = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessInner, QString("%1").arg(fTessInner));
    connect(itemTessInner, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTessInnerChanged);
    list.clear();
    list << itemTessInner;
    list << new QStandardItem(itemTessInner->toolTip());
    pItemMaterialType->appendRow(list);
    data.setValue(fTessInner);
    itemTessInner->setData(data, MetaTreeItemRoles::SurfaceTessInner);

    float fTessOuter = 1.0;
    MetaTreeItem *itemTessOuter = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessOuter, QString("%1").arg(fTessOuter));
    connect(itemTessOuter, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTessOuterChanged);
    list.clear();
    list << itemTessOuter;
    list << new QStandardItem(itemTessOuter->toolTip());
    pItemMaterialType->appendRow(list);
    data.setValue(fTessOuter);
    itemTessOuter->setData(data, MetaTreeItemRoles::SurfaceTessOuter);

    float fTriangleScale = 1.0;
    MetaTreeItem *itemTriangleScale = new MetaTreeItem(MetaTreeItemTypes::SurfaceTriangleScale, QString("%1").arg(fTriangleScale));
    connect(itemTriangleScale, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTriangleScaleChanged);
    list.clear();
    list << itemTriangleScale;
    list << new QStandardItem(itemTriangleScale->toolTip());
    pItemMaterialType->appendRow(list);
    data.setValue(fTriangleScale);
    itemTriangleScale->setData(data, MetaTreeItemRoles::SurfaceTriangleScale);

    MetaTreeItem* pItemShowNormals = new MetaTreeItem(MetaTreeItemTypes::ShowNormals, "Show normals");
    connect(pItemShowNormals, &MetaTreeItem::checkStateChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceNormalsChanged);
    pItemShowNormals->setCheckable(true);
    list.clear();
    list << pItemShowNormals;
    list << new QStandardItem("Show the normals");
    this->appendRow(list);

    MetaTreeItem *itemXTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateX, QString::number(0));
    itemXTrans->setEditable(true);
    connect(itemXTrans, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTranslationXChanged);
    list.clear();
    list << itemXTrans;
    list << new QStandardItem(itemXTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemYTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateY, QString::number(0));
    itemYTrans->setEditable(true);
    connect(itemYTrans, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTranslationYChanged);
    list.clear();
    list << itemYTrans;
    list << new QStandardItem(itemYTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemZTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateZ, QString::number(0));
    itemZTrans->setEditable(true);
    connect(itemZTrans, &MetaTreeItem::dataChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceTranslationZChanged);
    list.clear();
    list << itemZTrans;
    list << new QStandardItem(itemZTrans->toolTip());
    this->appendRow(list);

    //Init materials
    m_pRenderable3DEntity->addComponent(m_pMaterial);
}


//*************************************************************************************************************

QVariant AbstractSurfaceTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::setData(const QVariant& value, int role)
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

void AbstractSurfaceTreeItem::setVisible(bool state)
{
    for(int i = 0; i < m_pRenderable3DEntity->childNodes().size(); ++i) {
        m_pRenderable3DEntity->childNodes()[i]->setEnabled(state);
    }
    m_pRenderable3DEntity->setEnabled(state);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceAlphaChanged(const QVariant& fAlpha)
{
    if(fAlpha.canConvert<float>()) {
        m_pRenderable3DEntity->setMaterialParameter(fAlpha.toFloat(), "alpha");
    }
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTessInnerChanged(const QVariant& fTessInner)
{
    m_pRenderable3DEntity->setMaterialParameter(fTessInner.toFloat(), "innerTess");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTessOuterChanged(const QVariant& fTessOuter)
{
    m_pRenderable3DEntity->setMaterialParameter(fTessOuter.toFloat(), "outerTess");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTriangleScaleChanged(const QVariant& fTriangleScale)
{
    m_pRenderable3DEntity->setMaterialParameter(fTriangleScale.toFloat(), "triangleScale");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{    
    m_pRenderable3DEntity->setEnabled(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationXChanged(const QVariant& fTransX)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setX(fTransX.toFloat());
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationYChanged(const QVariant& fTransY)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setY(fTransY.toFloat());
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationZChanged(const QVariant& fTransZ)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setZ(fTransZ.toFloat());
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceColorChanged(const QVariant& color)
{
    QVariant data;
    MatrixX3f matNewVertColor = createVertColor(this->data(Data3DTreeModelItemRoles::SurfaceVert).value<MatrixX3f>(), color.value<QColor>());

    data.setValue(matNewVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceMaterialChanged(const QVariant& sMaterial)
{
//    m_pRenderable3DEntity->removeComponent(m_pTessMaterial);
//    m_pRenderable3DEntity->removeComponent(m_pMaterial);

//    if(sMaterial.toString() == "Phong Alpha") {
//        m_pRenderable3DEntity->addComponent(m_pMaterial);
//        m_pRenderable3DEntity->getCustomMesh()->setPrimitiveType(Qt3DRender::QGeometryRenderer::Triangles);
//    } else if(sMaterial.toString() == "Phong Alpha Tesselation") {
//        m_pRenderable3DEntity->addComponent(m_pTessMaterial);
//        m_pRenderable3DEntity->getCustomMesh()->setPrimitiveType(Qt3DRender::QGeometryRenderer::Patches);
//    }
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceNormalsChanged(const Qt::CheckState& checkState)
{
    m_pNormalMaterial->setEnabled(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

MatrixX3f AbstractSurfaceTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color) const
{
    MatrixX3f matColor(vertices.rows(),3);

    for(int i = 0; i < matColor.rows(); ++i) {
        matColor(i,0) = color.redF();
        matColor(i,1) = color.greenF();
        matColor(i,2) = color.blueF();
    }

    return matColor;
}
