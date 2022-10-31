//=============================================================================================================
/**
 * @file     fssurfacetreeitem.cpp
 * @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
 *           Gabriel B Motta <gabrielbenmotta@gmail.com>;
 *           Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lars Debor, Gabriel B Motta, Lorenz Esch. All rights reserved.
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
 * @brief    FsSurfaceTreeItem class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fssurfacetreeitem.h"
#include "../common/metatreeitem.h"
#include "../../3dhelpers/renderable3Dentity.h"
#include "../../materials/pervertexphongalphamaterial.h"
#include "../../materials/shownormalsmaterial.h"
#include "../../3dhelpers/custommesh.h"

#include <fs/label.h>
#include <fs/surface.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

#include <Eigen/Core>

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace Eigen;
using namespace FSLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FsSurfaceTreeItem::FsSurfaceTreeItem(Qt3DCore::QEntity *p3DEntityParent, int iType, const QString& text)
: AbstractMeshTreeItem(p3DEntityParent, iType, text)
, m_sColorInfoOrigin("Color from curvature")
{
    initItem();
}

//=============================================================================================================

void FsSurfaceTreeItem::initItem()
{
    this->setToolTip("Brain hemisphere surface item");

    //Use SortPolicy in frame graph
    this->removeComponent(m_pMaterial);
    m_pMaterial = new PerVertexPhongAlphaMaterial(true);
    this->addComponent(m_pMaterial);

    //Add surface meta information as item children
    QList<QStandardItem*> list;
    QVariant data;

    if(!m_pItemSurfColSulci) {
        m_pItemSurfColSulci = new MetaTreeItem(MetaTreeItemTypes::SurfaceColorSulci, "Sulci color");
    }

    connect(m_pItemSurfColSulci.data(), &MetaTreeItem::dataChanged,
            this, &FsSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    list << m_pItemSurfColSulci;
    list << new QStandardItem(m_pItemSurfColSulci->toolTip());
    m_pItemAppearanceOptions->appendRow(list);
    data.setValue(QColor(50,50,50));
    m_pItemSurfColSulci->setData(data, MetaTreeItemRoles::SurfaceColorSulci);
    m_pItemSurfColSulci->setData(data, Qt::DecorationRole);

    if(!m_pItemSurfColGyri) {
        m_pItemSurfColGyri = new MetaTreeItem(MetaTreeItemTypes::SurfaceColorGyri, "Gyri color");
    }

    connect(m_pItemSurfColGyri.data(), &MetaTreeItem::dataChanged,
            this, &FsSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged);
    list.clear();
    list << m_pItemSurfColGyri;
    list << new QStandardItem(m_pItemSurfColGyri->toolTip());
    m_pItemAppearanceOptions->appendRow(list);
    data.setValue(QColor(125,125,125));
    m_pItemSurfColGyri->setData(data, MetaTreeItemRoles::SurfaceColorGyri);
    m_pItemSurfColGyri->setData(data, Qt::DecorationRole);

    this->setAlpha(1.0f);
}

//=============================================================================================================

void FsSurfaceTreeItem::addData(const Surface& tSurface)
{
    //Create color from curvature information with default gyri and sulcus colors
    MatrixX4f matCurvatureColor = createCurvatureVertColor(tSurface.curv());

    //Set renderable 3D entity mesh and color data
    m_pCustomMesh->setMeshData(tSurface.rr(),
                               tSurface.nn(),
                               tSurface.tris(),
                               matCurvatureColor,
                               Qt3DRender::QGeometryRenderer::Triangles);
    this->setPosition(QVector3D(-tSurface.offset()(0), -tSurface.offset()(1), -tSurface.offset()(2)));

    //Add data which is held by this FsSurfaceTreeItem
    QVariant data;

    data.setValue(matCurvatureColor);
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

    data.setValue(tSurface.curv());
    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurv);

    data.setValue(tSurface.rr().rows());
    this->setData(data, Data3DTreeModelItemRoles::NumberVertices);

     //Add data which is held by this FsSurfaceTreeItem
    QList<QStandardItem*> list;

    MetaTreeItem *itemSurfFileName = new MetaTreeItem(MetaTreeItemTypes::FileName, tSurface.fileName());
    itemSurfFileName->setEditable(false);
    list.clear();
    list << itemSurfFileName;
    list << new QStandardItem(itemSurfFileName->toolTip());
    this->appendRow(list);
    data.setValue(tSurface.fileName());
    itemSurfFileName->setData(data, MetaTreeItemRoles::SurfaceFileName);

//    MetaTreeItem *itemSurfPath = new MetaTreeItem(MetaTreeItemTypes::FilePath, tSurface.filePath());
//    itemSurfPath->setEditable(false);
//    list.clear();
//    list << itemSurfPath;
//    list << new QStandardItem(itemSurfPath->toolTip());
//    this->appendRow(list);
//    data.setValue(tSurface.filePath());
//    itemSurfPath->setData(data, MetaTreeItemRoles::SurfaceFilePath);
}

//=============================================================================================================

void FsSurfaceTreeItem::onAnnotationVisibilityChanged(bool isVisible)
{
    if(isVisible) {
        m_sColorInfoOrigin = "Color from annotation";
    } else {
        m_sColorInfoOrigin = "Color from curvature";
    }

    onColorInfoOriginOrCurvColorChanged();
}

//=============================================================================================================

MatrixX4f FsSurfaceTreeItem::createCurvatureVertColor(const VectorXf& curvature,
                                                      const QColor& colSulci,
                                                      const QColor& colGyri)
{
    MatrixX4f colors(curvature.rows(), 4);

    for(int i = 0; i < colors.rows(); ++i) {
        //Color (this is the default color and will be used until the updateVertColor function was called)
        if(curvature(i) >= 0) {
            colors(i,0) = colSulci.redF();
            colors(i,1) = colSulci.greenF();
            colors(i,2) = colSulci.blueF();
            colors(i,3) = colSulci.alphaF();
        } else {
            colors(i,0) = colGyri.redF();
            colors(i,1) = colGyri.greenF();
            colors(i,2) = colGyri.blueF();
            colors(i,3) = colGyri.alphaF();
        }
    }

    return colors;
}

//=============================================================================================================

void FsSurfaceTreeItem::onColorInfoOriginOrCurvColorChanged()
{
    if(m_pItemSurfColSulci && m_pItemSurfColGyri) {
        QVariant data;
        MatrixX4f matNewVertColor;

        if(m_sColorInfoOrigin.contains("Color from curvature")) {
            //Create color from curvature information with default gyri and sulcus colors
            QColor colorSulci = m_pItemSurfColSulci->data(MetaTreeItemRoles::SurfaceColorSulci).value<QColor>();
            QColor colorGyri = m_pItemSurfColGyri->data(MetaTreeItemRoles::SurfaceColorGyri).value<QColor>();

            matNewVertColor = createCurvatureVertColor(this->data(Data3DTreeModelItemRoles::SurfaceCurv).value<VectorXf>(), colorSulci, colorGyri);

            data.setValue(matNewVertColor);
            this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

            //Return here because the new colors will be set to the renderable entity in the setData() function with the role Data3DTreeModelItemRoles::SurfaceCurrentColorVert
            return;
        }

        if(m_sColorInfoOrigin.contains("Color from annotation")) {
            //Find the FsAnnotationTreeItem
            for(int i = 0; i < this->QStandardItem::parent()->rowCount(); ++i) {
                if(this->QStandardItem::parent()->child(i,0)->type() == Data3DTreeModelItemTypes::AnnotationItem) {
                    matNewVertColor = this->QStandardItem::parent()->child(i,0)->data(Data3DTreeModelItemRoles::AnnotColors).value<MatrixX4f>();

                    //Set renderable 3D entity mesh and color data
                    data.setValue(matNewVertColor);
                    this->setData(data, Data3DTreeModelItemRoles::SurfaceCurrentColorVert);

                    //Return here because the new colors will be set to the renderable entity in the setData() function with the role Data3DTreeModelItemRoles::SurfaceCurrentColorVert
                    return;
                }
            }
        }
    }
}

