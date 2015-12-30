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
        m_pRenderable3DEntity->setVertColor(value.value<MatrixX3f>());
        break;
    }
}


//*************************************************************************************************************

bool BrainSurfaceTreeItem::addData(const Surface& tSurface, Qt3DCore::QEntity* parent)
{
    //Create renderable 3D entity
    m_pRenderable3DEntity = new Renderable3DEntity(parent);

    //Create color from curvature information with default gyri and sulcus colors
    MatrixX3f matCurvatureColor = createCurvatureVertColor(tSurface.curv());

    //Set renderable 3D entity mesh and color data
    m_pRenderable3DEntity->setMeshData(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset(), matCurvatureColor);

    //Add data which is held by this BrainSurfaceTreeItem
    QVariant data;

    data.setValue(matCurvatureColor);
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
        QColor colorSulci = m_pItemSurfColSulci->data(BrainTreeItemRoles::SurfaceColorSulci).value<QColor>();
        QColor colorGyri = m_pItemSurfColGyri->data(BrainTreeItemRoles::SurfaceColorGyri).value<QColor>();
        QVariant data;
        MatrixX3f matNewVertColor;

        if(sColorInfoOrigin == "Color from curvature") {
            //Create color from curvature information with default gyri and sulcus colors
            matNewVertColor = createCurvatureVertColor(this->data(BrainSurfaceTreeItemRoles::SurfaceCurv).value<VectorXf>(), colorSulci, colorGyri);

            data.setValue(matNewVertColor);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert);
            this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

            //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
            return;
        }

        if(sColorInfoOrigin == "Color from annotation") {
            //Find the BrainAnnotationTreeItem
            for(int i = 0; i<this->QStandardItem::parent()->rowCount(); i++) {
                if(this->QStandardItem::parent()->child(i,0)->type() == BrainTreeModelItemTypes::AnnotationItem) {
                    matNewVertColor = this->QStandardItem::parent()->child(i,0)->data(BrainAnnotationTreeItemRoles::AnnotColors).value<MatrixX3f>();

                    //Set renderable 3D entity mesh and color data
                    data.setValue(matNewVertColor);
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

void BrainSurfaceTreeItem::updateRtVertColor(const VectorXd& sample, const VectorXi& vertexIndex)
{
    qDebug()<<"sample.rows()"<<sample.rows();
    qDebug()<<"vertexIndex.rows()"<<vertexIndex.rows();

    if(sample.rows() != vertexIndex.rows()) {
        qDebug()<<"BrainSurfaceTreeItem::updateRtVertColor - number of rows in sample do not not match with idx/no number of rows in vertex. Returning...";
        return;
    }

    //This function is called for every new sample point and therfore must be kept highly efficient!
    QString sColorInfoOrigin = m_pItemSurfColorInfoOrigin->data(BrainTreeItemRoles::SurfaceColorInfoOrigin).toString();
    MatrixX3f matCurrentVertColor;

    if(sColorInfoOrigin == "Color from curvature") {
        matCurrentVertColor = this->data(BrainSurfaceTreeItemRoles::SurfaceCurvatureColorVert).value<MatrixX3f>();
    }

    if(sColorInfoOrigin == "Color from annotation") {
        matCurrentVertColor = this->data(BrainSurfaceTreeItemRoles::SurfaceAnnotationColorVert).value<MatrixX3f>();
    }

    for(int i = 0; i<sample.rows(); i++) {
        qint32 iVal = sample(i) * 20;

        iVal = iVal > 255 ? 255 : iVal < 0 ? 0 : iVal;

        QRgb qRgb;
        //qRgb = ColorMap::valueToHotNegative1((float)iVal/255.0);
        qRgb = ColorMap::valueToHotNegative2((float)iVal/255.0);
        //qRgb = ColorMap::valueToHot((float)iVal/255.0);

        QColor colSample(qRgb);
        matCurrentVertColor(vertexIndex(i), 0) = colSample.redF();
        matCurrentVertColor(vertexIndex(i), 1) = colSample.greenF();
        matCurrentVertColor(vertexIndex(i), 2) = colSample.blueF();
    }

    QVariant data;
    data.setValue(matCurrentVertColor);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceRTSourceLocColor);
    this->setData(data, BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert);

    //Return here because the new colors will be set to the renderable entity in the setData() function with the role BrainSurfaceTreeItemRoles::SurfaceCurrentColorVert
    return;
}

//*************************************************************************************************************

MatrixX3f BrainSurfaceTreeItem::createCurvatureVertColor(const VectorXf& curvature, const QColor& colSulci, const QColor& colGyri)
{
    MatrixX3f matCurvatureColor(curvature.rows(), 3);

    for(int i = 0; i<curvature.rows() ; i++) {
        if(curvature[i] >= 0) {
            matCurvatureColor(i, 0) = colSulci.redF();
            matCurvatureColor(i, 1) = colSulci.greenF();
            matCurvatureColor(i, 2) = colSulci.blueF();
        } else {
            matCurvatureColor(i, 0) = colGyri.redF();
            matCurvatureColor(i, 1) = colGyri.greenF();
            matCurvatureColor(i, 2) = colGyri.blueF();
        }
    }

    return matCurvatureColor;
}
