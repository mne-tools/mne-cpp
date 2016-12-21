//=============================================================================================================
/**
* @file     brainannotationtreeitem.cpp
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
* @brief    BrainAnnotationTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "brainannotationtreeitem.h"
#include "../common/metatreeitem.h"

#include <fs/label.h>
#include <fs/surface.h>
#include <fs/annotation.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QList>
#include <QVariant>
#include <QStringList>
#include <QColor>
#include <QStandardItem>
#include <QStandardItemModel>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;
using namespace FSLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainAnnotationTreeItem::BrainAnnotationTreeItem(int iType, const QString & text)
: AbstractTreeItem(iType, text)
{
    this->setEditable(false);
    this->setCheckable(true);
    this->setCheckState(Qt::Unchecked);
    this->setToolTip("Brain annotation item");
}


//*************************************************************************************************************

BrainAnnotationTreeItem::~BrainAnnotationTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainAnnotationTreeItem::data(int role) const
{
    return AbstractTreeItem::data(role);
}


//*************************************************************************************************************

void  BrainAnnotationTreeItem::setData(const QVariant& value, int role)
{
    AbstractTreeItem::setData(value, role);
}


//*************************************************************************************************************

void BrainAnnotationTreeItem::addData(const Surface& tSurface, const Annotation& tAnnotation)
{
    //Create color from annotation data if annotation is not empty
    if(!tAnnotation.isEmpty()) {
        QByteArray arrayColorsAnnot;
        arrayColorsAnnot.resize(tAnnotation.getVertices().rows() * 3 * (int)sizeof(float));
        float *rawArrayColors = reinterpret_cast<float *>(arrayColorsAnnot.data());

        QList<FSLIB::Label> qListLabels;
        QList<RowVector4i> qListLabelRGBAs;

        tAnnotation.toLabels(tSurface, qListLabels, qListLabelRGBAs);

        for(int i = 0; i < qListLabels.size(); ++i) {
            FSLIB::Label label = qListLabels.at(i);
            for(int j = 0; j<label.vertices.rows(); j++) {
                QColor patchColor;
                patchColor.setRed(qListLabelRGBAs.at(i)(0));
                patchColor.setGreen(qListLabelRGBAs.at(i)(1));
                patchColor.setBlue(qListLabelRGBAs.at(i)(2));

                patchColor = patchColor.darker(200);

                rawArrayColors[label.vertices(j)*3+0] = patchColor.redF();
                rawArrayColors[label.vertices(j)*3+1] = patchColor.greenF();
                rawArrayColors[label.vertices(j)*3+2] = patchColor.blueF();
            }
        }

        //Add data which is held by this BrainAnnotationTreeItem
        QVariant data;
        data.setValue(arrayColorsAnnot);
        this->setData(data, Data3DTreeModelItemRoles::AnnotColors);

        data.setValue(qListLabels);
        this->setData(data, Data3DTreeModelItemRoles::LabeList);

        data.setValue(tAnnotation.getLabelIds());
        this->setData(data, Data3DTreeModelItemRoles::LabeIds);

        //Add annotation meta information as item children
        QList<QStandardItem*> list;

        MetaTreeItem *itemAnnotFileName = new MetaTreeItem(MetaTreeItemTypes::FileName, tAnnotation.fileName());
        itemAnnotFileName->setEditable(false);
        list << itemAnnotFileName;
        list << new QStandardItem(itemAnnotFileName->toolTip());
        this->appendRow(list);
        data.setValue(tAnnotation.fileName());
        itemAnnotFileName->setData(data, Data3DTreeModelItemRoles::FileName);

        list.clear();
        MetaTreeItem *itemAnnotPath = new MetaTreeItem(MetaTreeItemTypes::FilePath, tAnnotation.filePath());
        itemAnnotPath->setEditable(false);
        list << itemAnnotPath;
        list << new QStandardItem(itemAnnotPath->toolTip());
        this->appendRow(list);
        data.setValue(tAnnotation.filePath());
        itemAnnotFileName->setData(data, Data3DTreeModelItemRoles::FilePath);
    }
}


//*************************************************************************************************************

void BrainAnnotationTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    emit annotationVisibiltyChanged(checkState==Qt::Unchecked ? false : true);
}

