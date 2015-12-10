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

bool BrainSurfaceTreeItem::addFsSurfData(const Surface& tSurface)
{
    //Set renderable 3D entity and mesh data
    this->setMeshData(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset());

    //Add surface meta information
    BrainTreeItem *itemSurfFileName = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileName, tSurface.fileName());
    this->appendRow(itemSurfFileName);

    BrainTreeItem *itemSurfType = new BrainTreeItem(BrainTreeItemTypes::SurfaceFileType, tSurface.surf());
    this->appendRow(itemSurfType);

    BrainTreeItem *itemSurfPath = new BrainTreeItem(BrainTreeItemTypes::SurfaceFilePath, tSurface.filePath());
    this->appendRow(itemSurfPath);

//    //Create color from curvature information
//    m_matColorsOrig.resize(m_matVert.rows(), m_matVert.cols());

//    for(int i = 0; i<m_matVert.rows() ; i++) {
//        if(m_vecCurv[i] >= 0) {
//            m_matColorsOrig(i, 0) = m_ColorSulci.redF();
//            m_matColorsOrig(i, 1) = m_ColorSulci.greenF();
//            m_matColorsOrig(i, 2) = m_ColorSulci.blueF();
//        } else {
//            m_matColorsOrig(i, 0) = m_ColorGyri.redF();
//            m_matColorsOrig(i, 1) = m_ColorGyri.greenF();
//            m_matColorsOrig(i, 2) = m_ColorGyri.blueF();
//        }
//    }

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

