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

BrainSurfaceTreeItem::BrainSurfaceTreeItem(const Surface &tSurface, const Annotation &tAnnotation, QString sDesc, Qt3DCore::QEntity *entityParent, AbstractTreeItem *treeItemParent)
: AbstractTreeItem(0, sDesc, treeItemParent)
, Renderable3DEntity(tSurface.rr(), tSurface.nn(), tSurface.tris(), -tSurface.offset(), entityParent)
{
}


//*************************************************************************************************************

BrainSurfaceTreeItem::~BrainSurfaceTreeItem()
{
}


//*************************************************************************************************************

QHash<int, QByteArray> BrainSurfaceTreeItem::roleNames() const
{
    QHash<int, QByteArray> roles;

    return roles;
}


//*************************************************************************************************************

int BrainSurfaceTreeItem::columnCount() const
{
    return m_lItemData.count();
}


//*************************************************************************************************************

QVariant BrainSurfaceTreeItem::data(int column, int role) const
{
    Q_UNUSED(role);

    return m_lItemData.value(column);
}


//*************************************************************************************************************

bool BrainSurfaceTreeItem::setData(int role, const QVariant &value)
{
    Q_UNUSED(role);

    return true;
}


//*************************************************************************************************************

int BrainSurfaceTreeItem::childCount() const
{
    return 5;
}
