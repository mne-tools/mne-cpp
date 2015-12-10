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

BrainSurfaceTreeItem::BrainSurfaceTreeItem(const int & iType, const QString & text, Qt3DCore::QEntity *parent)
: AbstractTreeItem(iType, text)
, Renderable3DEntity(parent)
{
    this->setToolTip("Brain hemisphere");
}


//*************************************************************************************************************

BrainSurfaceTreeItem::~BrainSurfaceTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSurfaceTreeItem::data(int role) const
{
    switch(role) {
        case BrainTreeModelRoles::GetSurfName:
            return QVariant();

        case BrainTreeModelRoles::GetRenderable3DEntity:
            return QVariant();

    }

    return QStandardItem::data(role);
}


//*************************************************************************************************************

void  BrainSurfaceTreeItem::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);
}


//*************************************************************************************************************

bool BrainSurfaceTreeItem::addFsData(const Surface& tSurface, const Annotation& tAnnotation)
{
    //Set name of this item based on the hemispehre information
    QString itemText;

    switch (tSurface.hemi()) {
    case 0:
        itemText = "Left hemisphere";
        break;
    case 1:
        itemText = "Right hemisphere";
        break;
    default:
        itemText = "Unknown hemisphere";
        break;
    }

    this->setText(itemText);

    //Set renderable 3D entity and mesh data
    this->setMeshData(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset());

    //Add meta information of this item
    BrainTreeItem *itemSurfFileName = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileName, tSurface.fileName());
    this->appendRow(itemSurfFileName);

    BrainTreeItem *itemSurfType = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileType, tSurface.surf());
    this->appendRow(itemSurfType);

    BrainTreeItem *itemSurfPath = new BrainTreeItem(BrainTreeItemTypes::SurfaceFilePath, tSurface.filePath());
    this->appendRow(itemSurfPath);

    BrainTreeItem *itemAnnotFileName = new BrainTreeItem(BrainTreeItemTypes::AnnotFileName, tAnnotation.fileName());
    this->appendRow(itemAnnotFileName);

    BrainTreeItem *itemAnnotPath = new BrainTreeItem(BrainTreeItemTypes::AnnotFilePath, tAnnotation.filePath());
    this->appendRow(itemAnnotPath);

    //    //ColorSulci
    //    lDataVariant<<QColor(50,50,50);
    //    BrainTreeItem* pBrainTreeItemColorSulci = new BrainTreeItem(lDataVariant, BrainTreeItem::ColorSulci, pBrainTreeItemTop);
    //    pBrainTreeItemTop->appendChild(pBrainTreeItemColorSulci);
    //    lDataVariant.clear();

    //    //ColorGyri
    //    lDataVariant<<QColor(125,125,125);
    //    BrainTreeItem* pBrainTreeItemColorGyri = new BrainTreeItem(lDataVariant, BrainTreeItem::ColorGyri, pBrainTreeItemTop);
    //    pBrainTreeItemTop->appendChild(pBrainTreeItemColorGyri);
    //    lDataVariant.clear();

    return true;
}

