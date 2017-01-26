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
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/pervertexphongalphamaterial.h"

#include <mne/mne_bem.h>
#include <fiff/fiff_constants.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QUrl>


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

BemSurfaceTreeItem::BemSurfaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("BEM surface item");
}


//*************************************************************************************************************

BemSurfaceTreeItem::~BemSurfaceTreeItem()
{
    //Schedule deletion/Decouple of all entities so that the SceneGraph is NOT plotting them anymore.
    if(!m_pRenderable3DEntity.isNull()) {
        m_pRenderable3DEntity->deleteLater();
    }
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
        default: // do nothing;
            break;
    }
}


//*************************************************************************************************************

void BemSurfaceTreeItem::addData(const MNEBemSurface& tBemSurface, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Create color from curvature information with default gyri and sulcus colors
    QByteArray arrayVertColor = createVertColor(tBemSurface.rr);

    //Set renderable 3D entity mesh and color data
    m_pRenderable3DEntity->setMeshData(tBemSurface.rr, tBemSurface.nn, tBemSurface.tris, arrayVertColor, Qt3DRender::QGeometryRenderer::Triangles);

    //Set shaders
    PerVertexPhongAlphaMaterial* pPerVertexPhongAlphaMaterial = new PerVertexPhongAlphaMaterial();
    m_pRenderable3DEntity->addComponent(pPerVertexPhongAlphaMaterial);

    //Find out BEM layer type and change items name
    this->setText(MNEBemSurface::id_name(tBemSurface.id));

    //Add data which is held by this BemSurfaceTreeItem
    QVariant data;

    data.setValue(arrayVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

    data.setValue(tBemSurface.rr);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceVert);

    //Add surface meta information as item children
    QList<QStandardItem*> list;

    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::SurfaceAlpha, "1.0");
    connect(itemAlpha, &MetaTreeItem::surfaceAlphaChanged,
            this, &BemSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list << itemAlpha;
    list << new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(0.5);
    itemAlpha->setData(data, MetaTreeItemRoles::SurfaceAlpha);

    MetaTreeItem* pItemSurfCol = new MetaTreeItem(MetaTreeItemTypes::SurfaceColor, "Surface color");
    connect(pItemSurfCol, &MetaTreeItem::surfaceColorChanged,
            this, &BemSurfaceTreeItem::onSurfaceColorChanged);
    list.clear();
    list << pItemSurfCol;
    list << new QStandardItem(pItemSurfCol->toolTip());
    this->appendRow(list);
    data.setValue(QColor(100,100,100));
    pItemSurfCol->setData(data, MetaTreeItemRoles::SurfaceColor);
    pItemSurfCol->setData(data, Qt::DecorationRole);

    MetaTreeItem *itemXTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateX, QString::number(0));
    itemXTrans->setEditable(true);
    connect(itemXTrans, &MetaTreeItem::surfaceTranslationXChanged,
            this, &BemSurfaceTreeItem::onSurfaceTranslationXChanged);
    list.clear();
    list << itemXTrans;
    list << new QStandardItem(itemXTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemYTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateY, QString::number(0));
    itemYTrans->setEditable(true);
    connect(itemYTrans, &MetaTreeItem::surfaceTranslationYChanged,
            this, &BemSurfaceTreeItem::onSurfaceTranslationYChanged);
    list.clear();
    list << itemYTrans;
    list << new QStandardItem(itemYTrans->toolTip());
    this->appendRow(list);

    MetaTreeItem *itemZTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateZ, QString::number(0));
    itemZTrans->setEditable(true);
    connect(itemZTrans, &MetaTreeItem::surfaceTranslationZChanged,
            this, &BemSurfaceTreeItem::onSurfaceTranslationZChanged);
    list.clear();
    list << itemZTrans;
    list << new QStandardItem(itemZTrans->toolTip());
    this->appendRow(list);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::setVisible(bool state)
{
    m_pRenderable3DEntity->setEnabled(state);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onSurfaceAlphaChanged(float fAlpha)
{
    m_pRenderable3DEntity->setMaterialParameter(fAlpha, "alpha");
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

void BemSurfaceTreeItem::onSurfaceTranslationXChanged(float fTransX)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setX(fTransX);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onSurfaceTranslationYChanged(float fTransY)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setY(fTransY);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

void BemSurfaceTreeItem::onSurfaceTranslationZChanged(float fTransZ)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setZ(fTransZ);
    m_pRenderable3DEntity->setPosition(position);
}


//*************************************************************************************************************

QByteArray BemSurfaceTreeItem::createVertColor(const MatrixXf& vertices, const QColor& color) const
{
    QByteArray arrayCurvatureColor;
    arrayCurvatureColor.resize(vertices.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(arrayCurvatureColor.data());
    int idxColor = 0;

    for(int i = 0; i < vertices.rows(); ++i) {
        rawColorArray[idxColor++] = color.redF();
        rawColorArray[idxColor++] = color.greenF();
        rawColorArray[idxColor++] = color.blueF();
    }

    return arrayCurvatureColor;
}
