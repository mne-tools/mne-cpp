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

BrainTreeItem::BrainTreeItem(int iType, const QString & text)
: QStandardItem(text)
, m_iType(iType)
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

//    roles[BrainTreeModelRoles::SurfName] = "SurfName";
//    roles[BrainTreeModelRoles::SurfType] = "SurfType";
//    roles[BrainTreeModelRoles::SurfHemi] = "SurfHemi";
//    roles[BrainTreeModelRoles::SurfColorSulci] = "SurfColorSulci";
//    roles[BrainTreeModelRoles::SurfColorGyri] = "SurfColorGyri";
//    roles[BrainTreeModelRoles::SurfColorVert] = "SurfColorVert";
//    roles[BrainTreeModelRoles::SurfVert] = "SurfVert";
//    roles[BrainTreeModelRoles::SurfTris] = "SurfTris";
//    roles[BrainTreeModelRoles::SurfNorm] = "SurfNorm";
//    roles[BrainTreeModelRoles::SurfCurv] = "SurfCurv";
//    roles[BrainTreeModelRoles::SurfOffset] = "SurfOffset";
//    roles[BrainTreeModelRoles::SurfFilePath] = "SurfFilePath";
//    roles[BrainTreeModelRoles::SurfAnnotName] = "SurfAnnotName";
//    roles[BrainTreeModelRoles::SurfAnnotFilePath] = "SurfAnnotFilePath";
//    roles[BrainTreeModelRoles::SurfColorAnnot] = "SurfColorAnnot";
//    roles[BrainTreeModelRoles::RootItem] = "RootItem";

    return roles;
}


//*************************************************************************************************************

QVariant BrainTreeItem::data(int role) const
{
    switch(role) {
        case Qt::DisplayRole:
            return QStandardItem::data(role);
            break;

    }

    return QVariant();
}


//*************************************************************************************************************

void  BrainTreeItem::setData(const QVariant& value, int role)
{
    Q_UNUSED(role);
}


//*************************************************************************************************************

int  BrainTreeItem::type() const
{
    return m_iType;
}
