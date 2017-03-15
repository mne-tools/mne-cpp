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
, m_pRenderable3DEntityNormals(new Renderable3DEntity())
, m_bUseTesselation(false)
, m_bRenderNormals(false)
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

    if(m_pRenderable3DEntityNormals) {
        m_pRenderable3DEntityNormals->deleteLater();
    }
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::initItem()
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Abstract Surface item");

    //Set shaders
    if(!m_bUseTesselation) {
        PerVertexPhongAlphaMaterial* pPerVertexPhongAlphaMaterial = new PerVertexPhongAlphaMaterial();
        m_pRenderable3DEntity->addComponent(pPerVertexPhongAlphaMaterial);
    } else {
        PerVertexTessPhongAlphaMaterial* pPerVertexTessPhongAlphaMaterial = new PerVertexTessPhongAlphaMaterial();
        m_pRenderable3DEntity->addComponent(pPerVertexTessPhongAlphaMaterial);
    }

//    if(m_bRenderNormals) {
//        //Render normals
//        m_pRenderable3DEntityNormals->getCustomMesh()->setMeshData(tSurface.rr(),
//                                                      tSurface.nn(),
//                                                      tSurface.tris(),
//                                                      arrayCurvatureColor,
//                                                      Qt3DRender::QGeometryRenderer::Triangles);

//        ShowNormalsMaterial* pShowNormalsMaterial = new ShowNormalsMaterial();
//        m_pRenderable3DEntityNormals->addComponent(pShowNormalsMaterial);

//        //Generate activation overlay surface
//        MatrixX3f overlayAdds = tSurface.rr();
//        for(int i = 0; i<tSurface.nn().rows(); i++) {
//            RowVector3f direction = tSurface.nn().row(i);
//            direction.normalize();

//            overlayAdds.row(i) = direction*0.0001;
//        }

//        m_pRenderable3DEntityNormals->setMeshData(tSurface.rr()+overlayAdds, tSurface.nn(), tSurface.tris(), arrayCurvatureColor);
//    }

    //Add surface meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    float fAlpha = 0.35f;
    MetaTreeItem *itemAlpha = new MetaTreeItem(MetaTreeItemTypes::AlphaValue, QString("%1").arg(fAlpha));
    connect(itemAlpha, &MetaTreeItem::alphaChanged,
            this, &AbstractSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list << itemAlpha;
    list << new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(fAlpha);
    itemAlpha->setData(data, MetaTreeItemRoles::AlphaValue);

    if(m_bUseTesselation) {
        float fTessInner = 1.0;
        MetaTreeItem *itemTessInner = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessInner, QString("%1").arg(fTessInner));
        connect(itemTessInner, &MetaTreeItem::surfaceTessInnerChanged,
                this, &AbstractSurfaceTreeItem::onSurfaceTessInnerChanged);
        list.clear();
        list << itemTessInner;
        list << new QStandardItem(itemTessInner->toolTip());
        this->appendRow(list);
        data.setValue(fTessInner);
        itemTessInner->setData(data, MetaTreeItemRoles::SurfaceTessInner);

        float fTessOuter = 1.0;
        MetaTreeItem *itemTessOuter = new MetaTreeItem(MetaTreeItemTypes::SurfaceTessOuter, QString("%1").arg(fTessOuter));
        connect(itemTessOuter, &MetaTreeItem::surfaceTessOuterChanged,
                this, &AbstractSurfaceTreeItem::onSurfaceTessOuterChanged);
        list.clear();
        list << itemTessOuter;
        list << new QStandardItem(itemTessOuter->toolTip());
        this->appendRow(list);
        data.setValue(fTessOuter);
        itemTessOuter->setData(data, MetaTreeItemRoles::SurfaceTessOuter);

        float fTriangleScale = 1.0;
        MetaTreeItem *itemTriangleScale = new MetaTreeItem(MetaTreeItemTypes::SurfaceTriangleScale, QString("%1").arg(fTriangleScale));
        connect(itemTriangleScale, &MetaTreeItem::surfaceTriangleScaleChanged,
                this, &AbstractSurfaceTreeItem::onSurfaceTriangleScaleChanged);
        list.clear();
        list << itemTriangleScale;
        list << new QStandardItem(itemTriangleScale->toolTip());
        this->appendRow(list);
        data.setValue(fTriangleScale);
        itemTriangleScale->setData(data, MetaTreeItemRoles::SurfaceTriangleScale);
    }
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
    m_pRenderable3DEntity->setEnabled(state);
    m_pRenderable3DEntityNormals->setEnabled(state);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceAlphaChanged(float fAlpha)
{
    m_pRenderable3DEntity->setMaterialParameter(fAlpha, "alpha");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTessInnerChanged(float fTessInner)
{
    m_pRenderable3DEntity->setMaterialParameter(fTessInner, "innerTess");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTessOuterChanged(float fTessOuter)
{
    m_pRenderable3DEntity->setMaterialParameter(fTessOuter, "outerTess");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTriangleScaleChanged(float fTriangleScale)
{
    m_pRenderable3DEntity->setMaterialParameter(fTriangleScale, "triangleScale");
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{    
    m_pRenderable3DEntity->setEnabled(checkState == Qt::Unchecked ? false : true);
    m_pRenderable3DEntityNormals->setEnabled(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationXChanged(float fTransX)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setX(fTransX);
    m_pRenderable3DEntity->setPosition(position);
    m_pRenderable3DEntityNormals->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationYChanged(float fTransY)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setY(fTransY);
    m_pRenderable3DEntity->setPosition(position);
    m_pRenderable3DEntityNormals->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceTranslationZChanged(float fTransZ)
{
    QVector3D position = m_pRenderable3DEntity->position();
    position.setZ(fTransZ);
    m_pRenderable3DEntity->setPosition(position);
    m_pRenderable3DEntityNormals->setPosition(position);
}


//*************************************************************************************************************

void AbstractSurfaceTreeItem::onSurfaceColorChanged(const QColor& color)
{
    QVariant data;
    MatrixX3f matNewVertColor = createVertColor(this->data(Data3DTreeModelItemRoles::SurfaceVert).value<MatrixX3f>(), color);

    data.setValue(matNewVertColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);
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
