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

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainSurfaceTreeItem::BrainSurfaceTreeItem(const int& iType, const QString& text)
: AbstractTreeItem(iType, text)
, m_pRenderable3DEntity(new Renderable3DEntity())
, m_pRenderable3DEntityActivationOverlay(new Renderable3DEntity())
, m_pItemSurfColorInfoOrigin(new BrainTreeItem())
, m_pItemSurfColGyri(new BrainTreeItem())
, m_pItemSurfColSulci(new BrainTreeItem())
{
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
        QTime time;
        time.start();
        m_pRenderable3DEntity->setVertColor(value.value<QByteArray>());
        int timepassed = time.elapsed();
        qDebug()<<"BrainSurfaceTreeItem::setData:setVertColor:"<<timepassed<<"msecs";

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
    m_pItemSurfColorInfoOrigin = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceColorInfoOrigin, "Color from curvature");
    connect(m_pItemSurfColorInfoOrigin, &BrainTreeItem::updateSurfaceVertColors,
            this, &BrainSurfaceTreeItem::updateVertColor);
    *this<<m_pItemSurfColorInfoOrigin;
    data.setValue(QString("Color from curvature"));
    m_pItemSurfColorInfoOrigin->setData(data, BrainTreeItemRoles::SurfaceColorInfoOrigin);

    m_pItemSurfColSulci = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceColorSulci, "Sulci color");
    connect(m_pItemSurfColSulci, &BrainTreeItem::updateSurfaceVertColors,
            this, &BrainSurfaceTreeItem::updateVertColor);
    *this<<m_pItemSurfColSulci;
    data.setValue(QColor(50,50,50));
    m_pItemSurfColSulci->setData(data, BrainTreeItemRoles::SurfaceColorSulci);
    m_pItemSurfColSulci->setData(data, Qt::DecorationRole);

    m_pItemSurfColGyri = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceColorGyri, "Gyri color");
    connect(m_pItemSurfColGyri, &BrainTreeItem::updateSurfaceVertColors,
            this, &BrainSurfaceTreeItem::updateVertColor);
    *this<<m_pItemSurfColGyri;
    data.setValue(QColor(125,125,125));
    m_pItemSurfColGyri->setData(data, BrainTreeItemRoles::SurfaceColorGyri);
    m_pItemSurfColGyri->setData(data, Qt::DecorationRole);

    BrainTreeItem *itemSurfFileName = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceFileName, tSurface.fileName());
    *this<<itemSurfFileName;
    data.setValue(tSurface.fileName());
    itemSurfFileName->setData(data, BrainTreeItemRoles::SurfaceFileName);

    BrainTreeItem *itemSurfType = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceType, tSurface.surf());
    *this<<itemSurfType;
    data.setValue(tSurface.surf());
    itemSurfType->setData(data, BrainTreeItemRoles::SurfaceType);

    BrainTreeItem *itemSurfPath = new BrainTreeItem(BrainTreeModelItemTypes::SurfaceFilePath, tSurface.filePath());
    *this<<itemSurfPath;
    data.setValue(tSurface.filePath());
    itemSurfPath->setData(data, BrainTreeItemRoles::SurfaceFilePath);

    return true;
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::updateVertColor()
{
    if(this->hasChildren()) {
        QString sColorInfoOrigin = m_pItemSurfColorInfoOrigin->data(BrainTreeItemRoles::SurfaceColorInfoOrigin).toString();
        QVariant data;
        QByteArray arrayNewVertColor;

        if(sColorInfoOrigin == "Color from curvature") {
            //Create color from curvature information with default gyri and sulcus colors
            QColor colorSulci = m_pItemSurfColSulci->data(BrainTreeItemRoles::SurfaceColorSulci).value<QColor>();
            QColor colorGyri = m_pItemSurfColGyri->data(BrainTreeItemRoles::SurfaceColorGyri).value<QColor>();

            arrayNewVertColor = createCurvatureVertColor(this->data(BrainSurfaceTreeItemRoles::SurfaceCurv).value<VectorXf>(), colorSulci, colorGyri);

            data.setValue(arrayNewVertColor);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

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

                    //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
                    return;
                }
            }
        }
    }
}


//*************************************************************************************************************

void BrainSurfaceTreeItem::updateRtVertColor(const QByteArray& sourceColorSamples, const VectorXi& vertexIndex)
{
    QTime time;
    time.start();

    if((sourceColorSamples.size()/((int)sizeof(float)*3)) != vertexIndex.rows()) {
        qDebug()<<"BrainSurfaceTreeItem::updateRtVertColor - number of rows in sample ("<<sourceColorSamples.size()/((int)sizeof(float)*3)<<") do not not match with idx/no number of rows in vertex ("<<vertexIndex.rows()<<"). Returning...";
        return;
    }

    //This function is called for every new sample point and therfore it must be kept highly efficient!
    QString sColorInfoOrigin = m_pItemSurfColorInfoOrigin->data(BrainTreeItemRoles::SurfaceColorInfoOrigin).toString();
    QByteArray arrayCurrentVertColor;

    if(sColorInfoOrigin == "Color from curvature") {
        arrayCurrentVertColor = this->data(BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert).value<QByteArray>();
    }

    if(sColorInfoOrigin == "Color from annotation") {
        arrayCurrentVertColor = this->data(BrainSurfaceTreeItemRoles::SurfaceAnnotationColorVert).value<QByteArray>();
    }

    //Create final QByteArray with colors based on the current anatomical information
    const float *rawSourceColorSamples = reinterpret_cast<const float *>(sourceColorSamples.data());
    int idxSourceColorSamples = 0;
    float *rawArrayCurrentVertColor = reinterpret_cast<float *>(arrayCurrentVertColor.data());

    for(int i = 0; i<vertexIndex.rows(); i++) {
        rawArrayCurrentVertColor[vertexIndex(i)*3+0] = rawSourceColorSamples[idxSourceColorSamples++];
        rawArrayCurrentVertColor[vertexIndex(i)*3+1] = rawSourceColorSamples[idxSourceColorSamples++];
        rawArrayCurrentVertColor[vertexIndex(i)*3+2] = rawSourceColorSamples[idxSourceColorSamples++];
    }

    //Set new data. Here we also pass the new color values to the renderer (see setData function)
    QVariant data;
    data.setValue(arrayCurrentVertColor);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceRTSourceLocColor);

    int timepassed = time.elapsed();
    qDebug()<<"BrainSurfaceTreeItem::updateRtVertColor"<<timepassed<<"msecs";

    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

    return;
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
