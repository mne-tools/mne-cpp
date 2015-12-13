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

BrainSurfaceTreeItem::BrainSurfaceTreeItem(const int& iType, const QString& text, Qt3DCore::QEntity* parent)
: AbstractTreeItem(iType, text)
, Renderable3DEntity(parent)
{
}


//*************************************************************************************************************

BrainSurfaceTreeItem::~BrainSurfaceTreeItem()
{
}


//*************************************************************************************************************

QVariant BrainSurfaceTreeItem::data(int role) const
{
    switch(role) {
        case BrainTreeModelRoles::SurfaceName:
            return QVariant();

        case BrainTreeModelRoles::Renderable3DEntity:
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

bool BrainSurfaceTreeItem::addFsSurfData(const Surface& tSurface)
{
    //Create color from curvature information
    Matrix<float, Dynamic, 3, RowMajor> matColorsOrig(tSurface.rr().rows(), tSurface.rr().cols());
    QColor colSulci(50,50,50);
    QColor colGyri(125,125,125);

    for(int i = 0; i<matColorsOrig.rows() ; i++) {
        if(tSurface.curv()[i] >= 0) {
            matColorsOrig(i, 0) = colSulci.redF();
            matColorsOrig(i, 1) = colSulci.greenF();
            matColorsOrig(i, 2) = colSulci.blueF();
        } else {
            matColorsOrig(i, 0) = colGyri.redF();
            matColorsOrig(i, 1) = colGyri.greenF();
            matColorsOrig(i, 2) = colGyri.blueF();
        }
    }

    this->setVertColor(matColorsOrig);

    //Set renderable 3D entity and mesh data
    this->setMeshData(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset());

    //Add surface meta information
    BrainTreeItem *itemSurfColSulci = new BrainTreeItem(BrainTreeItemTypes::SurfaceColorSulci, "Color sulci");
    *this<<itemSurfColSulci;

    BrainTreeItem *itemSurfColGyri = new BrainTreeItem(BrainTreeItemTypes::SurfaceColorGyri, "Color gyri");
    *this<<itemSurfColGyri;

    BrainTreeItem *itemSurfFileName = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileName, tSurface.fileName());
    *this<<itemSurfFileName;

    BrainTreeItem *itemSurfType = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileType, tSurface.surf());
    *this<<itemSurfType;

    BrainTreeItem *itemSurfPath = new BrainTreeItem(BrainTreeItemTypes::SurfaceFilePath, tSurface.filePath());
    *this<<itemSurfPath;

    return true;
}

