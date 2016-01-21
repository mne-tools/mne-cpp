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
    this->setToolTip("Brain annotation");
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

bool BrainAnnotationTreeItem::addData(const Surface& tSurface, const Annotation& tAnnotation)
{
    //Create color from annotation data if annotation is not empty
    if(!tAnnotation.isEmpty()) {
        QByteArray arrayColorsAnnot;
        arrayColorsAnnot.resize(tAnnotation.getVertices().rows() * 3 * (int)sizeof(float));
        float *rawArrayColors = reinterpret_cast<float *>(arrayColorsAnnot.data());

        QList<FSLIB::Label> qListLabels;
        QList<RowVector4i> qListLabelRGBAs;

        tAnnotation.toLabels(tSurface, qListLabels, qListLabelRGBAs);

        for(int i = 0; i<qListLabels.size(); i++) {
            FSLIB::Label label = qListLabels.at(i);
            for(int j = 0; j<label.vertices.rows(); j++) {
                rawArrayColors[label.vertices(j)*3+0] = qListLabelRGBAs.at(i)(0)/255.0;
                rawArrayColors[label.vertices(j)*3+1] = qListLabelRGBAs.at(i)(1)/255.0;
                rawArrayColors[label.vertices(j)*3+2] = qListLabelRGBAs.at(i)(2)/255.0;
            }
        }

        //Add data which is held by this BrainAnnotationTreeItem
        QVariant data;
        data.setValue(arrayColorsAnnot);
        this->setData(data, BrainAnnotationTreeItemRoles::AnnotColors);

        data.setValue(qListLabels);
        this->setData(data, BrainAnnotationTreeItemRoles::LabeList);

        data.setValue(tAnnotation.getLabelIds());
        this->setData(data, BrainAnnotationTreeItemRoles::LabeIds);

        //Add annotation meta information as item children
        BrainTreeMetaItem *itemAnnotFileName = new BrainTreeMetaItem(BrainTreeMetaItemTypes::AnnotFileName, tAnnotation.fileName());
        itemAnnotFileName->setEditable(false);
        *this<<itemAnnotFileName;
        data.setValue(tAnnotation.fileName());
        itemAnnotFileName->setData(data, BrainAnnotationTreeItemRoles::AnnotFileName);

        BrainTreeMetaItem *itemAnnotPath = new BrainTreeMetaItem(BrainTreeMetaItemTypes::AnnotFilePath, tAnnotation.filePath());
        itemAnnotPath->setEditable(false);
        *this<<itemAnnotPath;
        data.setValue(tAnnotation.filePath());
        itemAnnotFileName->setData(data, BrainAnnotationTreeItemRoles::AnnotFilePath);
    }

    return true;
}

