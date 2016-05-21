//=============================================================================================================
/**
* @file     brainsurfacetreeitem.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2015
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
* @brief    BrainSurfaceTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainsurfacetreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurfaceTreeItem::BrainSurfaceTreeItem(int iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_sColorInfoOrigin("Color from curvature")
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
, m_pRenderable3DEntityActivationOverlay(new Renderable3DEntity())
, m_pItemSurfColGyri(new BrainTreeMetaItem())
, m_pItemSurfColSulci(new BrainTreeMetaItem())
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Checked);
    this->setToolTip("Brain surface");
}


//*************************************************************************************************************

BrainSurfaceTreeItem::~BrainSurfaceTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSurfaceTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainSurfaceTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);

    switch(role) {
    case BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert:
        m_pRenderable3DEntity->setVertColor(value.value<QByteArray>());
        break;

//    case BrainSurfaceTreeItemRoles::SurfaceRTSourceLocColor:
//        m_pRenderable3DEntityActivationOverlay->setVertColor(value.value<MatrixX3f>());
//        break;
    }
}


//*************************************************************************************************************

bool BrainSurfaceTreeItem::addData(const Surface& tSurface, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pParentEntity = parent;
    m_pRenderable3DEntity = new Renderable3DEntity(parent);
    m_pRenderable3DEntityActivationOverlay = new Renderable3DEntity(parent);

    if(tSurface.hemi() == -1) {
        QMatrix4x4 m;
        Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
        m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
        m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
        m.translate(-0.035f,-0.01f,0.04f);
        m.scale(0.65f);
        transform->setMatrix(m);
        m_pRenderable3DEntity->addComponent(transform);
        m_pRenderable3DEntityActivationOverlay->addComponent(transform);
    } else {
        QMatrix4x4 m;
        Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
        m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
        m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
        transform->setMatrix(m);
        m_pRenderable3DEntity->addComponent(transform);
        m_pRenderable3DEntityActivationOverlay->addComponent(transform);
    }


    //Create color from curvature information with default gyri and sulcus colors
    QByteArray arrayCurvatureColor = createCurvatureVertColor(tSurface.curv());

    //Set renderable 3D entity mesh and color data
    m_pRenderable3DEntity->setMeshData(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset(), arrayCurvatureColor);

    //Generate activation overlay surface
//    MatrixX3f overlayAdds = tSurface.rr();
//    for(int i = 0; i<tSurface.nn().rows(); i++) {
//        RowVector3f direction = tSurface.nn().row(i);
//        direction.normalize();

//        overlayAdds.row(i) = direction*0.0001;
//    }

//    m_pRenderable3DEntityActivationOverlay->setMeshData(tSurface.rr()+overlayAdds, tSurface.nn(), tSurface.tris(), -tSurface.offset(), matCurvatureColor);

    //Add data which is held by this BrainSurfaceTreeItem
    QVariant data;

    data.setValue(arrayCurvatureColor);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert);

    data.setValue(tSurface.rr());
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceVert);

    data.setValue(tSurface.tris());
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceTris);

    data.setValue(tSurface.nn());
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceNorm);

    data.setValue(tSurface.curv());
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurv);

    data.setValue(tSurface.offset());
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceOffset);

    data.setValue(m_pRenderable3DEntity);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceRenderable3DEntity);

    data.setValue(m_pRenderable3DEntityActivationOverlay);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceRenderable3DEntityAcivationOverlay);

    //Add surface meta information as item children
    QList<QStandardItem*> list;

    m_pItemSurfColSulci = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceColorSulci, "Sulci color");
    connect(m_pItemSurfColSulci, &BrainTreeMetaItem::curvColorsChanged,
            this, &BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    list<<m_pItemSurfColSulci;
    list<<new QStandardItem(m_pItemSurfColSulci->toolTip());
    this->appendRow(list);
    data.setValue(QColor(50,50,50));
    m_pItemSurfColSulci->setData(data, BrainTreeMetaItemRoles::SurfaceColorSulci);
    m_pItemSurfColSulci->setData(data, Qt::DecorationRole);

    m_pItemSurfColGyri = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceColorGyri, "Gyri color");
    connect(m_pItemSurfColGyri, &BrainTreeMetaItem::curvColorsChanged,
            this, &BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    list.clear();
    list<<m_pItemSurfColGyri;
    list<<new QStandardItem(m_pItemSurfColGyri->toolTip());
    this->appendRow(list);
    data.setValue(QColor(125,125,125));
    m_pItemSurfColGyri->setData(data, BrainTreeMetaItemRoles::SurfaceColorGyri);
    m_pItemSurfColGyri->setData(data, Qt::DecorationRole);

    BrainTreeMetaItem *itemAlpha = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceAlpha, "0.5");
    connect(itemAlpha, &BrainTreeMetaItem::surfaceAlphaChanged,
            this, &BrainSurfaceTreeItem::onSurfaceAlphaChanged);
    list.clear();
    list<<itemAlpha;
    list<<new QStandardItem(itemAlpha->toolTip());
    this->appendRow(list);
    data.setValue(0.5);
    itemAlpha->setData(data, BrainTreeMetaItemRoles::SurfaceAlpha);

    BrainTreeMetaItem *itemSurfFileName = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceFileName, tSurface.fileName());
    itemSurfFileName->setEditable(false);
    list.clear();
    list<<itemSurfFileName;
    list<<new QStandardItem(itemSurfFileName->toolTip());
    this->appendRow(list);
    data.setValue(tSurface.fileName());
    itemSurfFileName->setData(data, BrainTreeMetaItemRoles::SurfaceFileName);

    BrainTreeMetaItem *itemSurfType = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceType, tSurface.surf());
    itemSurfType->setEditable(false);
    list.clear();
    list<<itemSurfType;
    list<<new QStandardItem(itemSurfType->toolTip());
    this->appendRow(list);
    data.setValue(tSurface.surf());
    itemSurfType->setData(data, BrainTreeMetaItemRoles::SurfaceType);

    BrainTreeMetaItem *itemSurfPath = new BrainTreeMetaItem(Data3DTreeMetaItemTypes::SurfaceFilePath, tSurface.filePath());
    itemSurfPath->setEditable(false);
    list.clear();
    list<<itemSurfPath;
    list<<new QStandardItem(itemSurfPath->toolTip());
    this->appendRow(list);
    data.setValue(tSurface.filePath());
    itemSurfPath->setData(data, BrainTreeMetaItemRoles::SurfaceFilePath);

    return true;
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onRtVertColorChanged(const QByteArray& sourceColorSamples)
{
    //Set new data.
    //In setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert) we pass the new color values to the renderer (see setData function).
    QVariant data;
    data.setValue(sourceColorSamples);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceRTSourceLocColor);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onAnnotationVisibilityChanged(bool isVisible)
{
    if(isVisible) {
        m_sColorInfoOrigin = "Color from annotation";
    } else {
        m_sColorInfoOrigin = "Color from curvature";
    }

    onColorInfoOriginOrCurvColorChanged();
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::setVisible(bool state)
{
    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged()
{
    if(this->hasChildren()) {
        QVariant data;
        QByteArray arrayNewVertColor;

        if(m_sColorInfoOrigin.contains("Color from curvature")) {
            //Create color from curvature information with default gyri and sulcus colors
            QColor colorSulci = m_pItemSurfColSulci->data(BrainTreeMetaItemRoles::SurfaceColorSulci).value<QColor>();
            QColor colorGyri = m_pItemSurfColGyri->data(BrainTreeMetaItemRoles::SurfaceColorGyri).value<QColor>();

            arrayNewVertColor = createCurvatureVertColor(this->data(BrainSurfaceTreeItemRoles::SurfaceCurv).value<VectorXf>(), colorSulci, colorGyri);

            data.setValue(arrayNewVertColor);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

            //Emit the new colors which are to be used during rt source loc plotting
            emit colorInfoOriginChanged(arrayNewVertColor);

            //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
            return;
        }

        if(m_sColorInfoOrigin.contains("Color from annotation")) {
            //Find the BrainAnnotationTreeItem
            for(int i = 0; i<this->QStandardItem::parent()->rowCount(); i++) {
                if(this->QStandardItem::parent()->child(i,0)->type() == Data3DTreeModelItemTypes::AnnotationItem) {
                    arrayNewVertColor = this->QStandardItem::parent()->child(i,0)->data(BrainAnnotationTreeItemRoles::AnnotColors).value<QByteArray>();

                    //Set renderable 3D entity mesh and color data
                    data.setValue(arrayNewVertColor);
                    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceAnnotationColorVert);
                    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

                    //Emit the new colors which are to be used during rt source loc plotting
                    emit colorInfoOriginChanged(arrayNewVertColor);

                    //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
                    return;
                }
            }
        }
    }
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onSurfaceAlphaChanged(float fAlpha)
{
    m_pRenderable3DEntity->setAlpha(fAlpha);
    m_pRenderable3DEntityActivationOverlay->setAlpha(fAlpha);
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    this->setVisible(checkState==Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

QByteArray BrainSurfaceTreeItem::createCurvatureVertColor(const VectorXf& curvature, const QColor& colSulci, const QColor& colGyri)
{
    QByteArray arrayCurvatureColor;
    arrayCurvatureColor.resize(curvature.rows() * 3 * (int)sizeof(float));
    float *rawColorArray = reinterpret_cast<float *>(arrayCurvatureColor.data());
    int idxColor = 0;

    for(int i = 0; i<curvature.rows(); i++) {
        //Color (this is the default color and will be used until the updateVertColor function was called)
        if(curvature[i] >= 0) {
            rawColorArray[idxColor++] = colSulci.redF();
            rawColorArray[idxColor++] = colSulci.greenF();
            rawColorArray[idxColor++] = colSulci.blueF();
        } else {
            rawColorArray[idxColor++] = colGyri.redF();
            rawColorArray[idxColor++] = colGyri.greenF();
            rawColorArray[idxColor++] = colGyri.blueF();
        }
    }

    return arrayCurvatureColor;
}
