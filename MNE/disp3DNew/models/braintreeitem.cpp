//=============================================================================================================
/**
* @file     braintreeitem.cpp
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
* @brief    BrainTreeItem class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "braintreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeItem::BrainTreeItem(QList<QVariant> lItemData, int iDataRole, QString sDesc, AbstractTreeItem* parent)
: AbstractTreeItem(iDataRole, sDesc, parent)
, m_lItemData(lItemData)
{
}


//*************************************************************************************************************

BrainTreeItem::~BrainTreeItem()
{
}


//*************************************************************************************************************

QHash<int, QByteArray> BrainTreeItem::roleNames() const
{
    QHash<int, QByteArray> roles;

    roles[BrainTreeItemModelRoles::SurfName] = "SurfName";
    roles[BrainTreeItemModelRoles::SurfType] = "SurfType";
    roles[BrainTreeItemModelRoles::SurfHemi] = "SurfHemi";
    roles[BrainTreeItemModelRoles::SurfColorSulci] = "SurfColorSulci";
    roles[BrainTreeItemModelRoles::SurfColorGyri] = "SurfColorGyri";
    roles[BrainTreeItemModelRoles::SurfColorVert] = "SurfColorVert";
    roles[BrainTreeItemModelRoles::SurfVert] = "SurfVert";
    roles[BrainTreeItemModelRoles::SurfTris] = "SurfTris";
    roles[BrainTreeItemModelRoles::SurfNorm] = "SurfNorm";
    roles[BrainTreeItemModelRoles::SurfCurv] = "SurfCurv";
    roles[BrainTreeItemModelRoles::SurfOffset] = "SurfOffset";
    roles[BrainTreeItemModelRoles::SurfFilePath] = "SurfFilePath";
    roles[BrainTreeItemModelRoles::SurfAnnotName] = "SurfAnnotName";
    roles[BrainTreeItemModelRoles::SurfAnnotFilePath] = "SurfAnnotFilePath";
    roles[BrainTreeItemModelRoles::SurfColorAnnot] = "SurfColorAnnot";
    roles[BrainTreeItemModelRoles::RootItem] = "RootItem";

    return roles;
}


//*************************************************************************************************************

int BrainTreeItem::columnCount() const
{
    return m_lItemData.count();
}


//*************************************************************************************************************

QVariant BrainTreeItem::data(int column, int role) const
{
    Q_UNUSED(role);

    return m_lItemData.value(column);
}


//*************************************************************************************************************

bool BrainTreeItem::setData(int role, const QVariant &value)
{
    Q_UNUSED(role);

    return true;
}
