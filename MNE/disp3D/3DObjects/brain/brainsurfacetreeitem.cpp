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
, m_pParentEntity(new Qt3DCore::QEntity())
, m_pRenderable3DEntity(new Renderable3DEntity())
, m_pRenderable3DEntityActivationOverlay(new Renderable3DEntity())
, m_pItemSurfColorInfoOrigin(new BrainTreeMetaItem())
, m_pItemSurfColGyri(new BrainTreeMetaItem())
, m_pItemSurfColSulci(new BrainTreeMetaItem())
{
    this->setEditable(false);
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

    QMatrix4x4 m;
    Qt3DCore::QTransform* transform =  new Qt3DCore::QTransform();
    m.rotate(180, QVector3D(0.0f, 1.0f, 0.0f));
    m.rotate(-90, QVector3D(1.0f, 0.0f, 0.0f));
    transform->setMatrix(m);
    m_pRenderable3DEntity->addComponent(transform);
    m_pRenderable3DEntityActivationOverlay->addComponent(transform);

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
    m_pItemSurfColorInfoOrigin = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceColorInfoOrigin, "Color from curvature");
    connect(m_pItemSurfColorInfoOrigin, &BrainTreeMetaItem::colorInfoOriginChanged,
            this, &BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    *this<<m_pItemSurfColorInfoOrigin;
    data.setValue(QString("Color from curvature"));
    m_pItemSurfColorInfoOrigin->setData(data, BrainTreeMetaItemRoles::SurfaceColorInfoOrigin);

    m_pItemSurfColSulci = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceColorSulci, "Sulci color");
    connect(m_pItemSurfColSulci, &BrainTreeMetaItem::curvColorsChanged,
            this, &BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    *this<<m_pItemSurfColSulci;
    data.setValue(QColor(50,50,50));
    m_pItemSurfColSulci->setData(data, BrainTreeMetaItemRoles::SurfaceColorSulci);
    m_pItemSurfColSulci->setData(data, Qt::DecorationRole);

    m_pItemSurfColGyri = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceColorGyri, "Gyri color");
    connect(m_pItemSurfColGyri, &BrainTreeMetaItem::curvColorsChanged,
            this, &BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    *this<<m_pItemSurfColGyri;
    data.setValue(QColor(125,125,125));
    m_pItemSurfColGyri->setData(data, BrainTreeMetaItemRoles::SurfaceColorGyri);
    m_pItemSurfColGyri->setData(data, Qt::DecorationRole);

    BrainTreeMetaItem *itemAlpha = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceAlpha, "0.5");
    connect(itemAlpha, &BrainTreeMetaItem::surfaceAlphaChanged,
            this, &BrainSurfaceTreeItem::onSurfaceAlphaChanged);
    itemAlpha->setEditable(true);
    *this<<itemAlpha;
    data.setValue(0.5);
    itemAlpha->setData(data, BrainTreeMetaItemRoles::SurfaceAlpha);

    BrainTreeMetaItem *itemSurfFileName = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceFileName, tSurface.fileName());
    itemSurfFileName->setEditable(false);
    *this<<itemSurfFileName;
    data.setValue(tSurface.fileName());
    itemSurfFileName->setData(data, BrainTreeMetaItemRoles::SurfaceFileName);

    BrainTreeMetaItem *itemSurfType = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceType, tSurface.surf());
    itemSurfType->setEditable(false);
    *this<<itemSurfType;
    data.setValue(tSurface.surf());
    itemSurfType->setData(data, BrainTreeMetaItemRoles::SurfaceType);

    BrainTreeMetaItem *itemSurfPath = new BrainTreeMetaItem(BrainTreeMetaItemTypes::SurfaceFilePath, tSurface.filePath());
    itemSurfPath->setEditable(false);
    *this<<itemSurfPath;
    data.setValue(tSurface.filePath());
    itemSurfPath->setData(data, BrainTreeMetaItemRoles::SurfaceFilePath);

    return true;
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged()
{
    if(this->hasChildren()) {
        QString sColorInfoOrigin = m_pItemSurfColorInfoOrigin->data(BrainTreeMetaItemRoles::SurfaceColorInfoOrigin).toString();
        QVariant data;
        QByteArray arrayNewVertColor;

        if(sColorInfoOrigin == "Color from curvature") {
            //Create color from curvature information with default gyri and sulcus colors
            QColor colorSulci = m_pItemSurfColSulci->data(BrainTreeMetaItemRoles::SurfaceColorSulci).value<QColor>();
            QColor colorGyri = m_pItemSurfColGyri->data(BrainTreeMetaItemRoles::SurfaceColorGyri).value<QColor>();

            arrayNewVertColor = createCurvatureVertColor(this->data(BrainSurfaceTreeItemRoles::SurfaceCurv).value<VectorXf>(), colorSulci, colorGyri);

            data.setValue(arrayNewVertColor);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

            emit colorInfoOriginChanged(arrayNewVertColor);

            //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
            return;
        }

        if(sColorInfoOrigin == "Color from annotation") {
            //Find the BrainAnnotationTreeItem
            for(int i = 0; i<this->QStandardItem::parent()->rowCount(); i++) {
                if(this->QStandardItem::parent()->child(i,0)->type() == BrainTreeModelItemTypes::AnnotationItem) {
                    arrayNewVertColor = this->QStandardItem::parent()->child(i,0)->data(BrainAnnotationTreeItemRoles::AnnotColors).value<QByteArray>();

                    //Set renderable 3D entity mesh and color data
                    data.setValue(arrayNewVertColor);
                    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceAnnotationColorVert);
                    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

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

void BrainSurfaceTreeItem::setVisible(bool state)
{
    m_pRenderable3DEntity->setParent(state ? m_pParentEntity : Q_NULLPTR);
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
