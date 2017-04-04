//=============================================================================================================
/**
* @file     abstract3Dtreeitem.cpp
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
* @brief    Abstract3DTreeItem class definition.
*
*/
//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstract3Dtreeitem.h"
#include "../common/metatreeitem.h"


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Abstract3DTreeItem::Abstract3DTreeItem(QEntity* p3DEntityParent, int iType, const QString& text)
: QStandardItem(text)
, Renderable3DEntity(p3DEntityParent)
, m_iType(iType)
{
    initItem();
}


//*************************************************************************************************************

void Abstract3DTreeItem::initItem()
{
    this->setToolTip("Abstract 3D Tree Item");

    //Transformation
    QList<QStandardItem*> list;

    MetaTreeItem* pItemTransformationOptions = new MetaTreeItem(MetaTreeItemTypes::UnknownItem, "Transformation");
    pItemTransformationOptions->setEditable(false);
    list.clear();
    list << pItemTransformationOptions;
    list << new QStandardItem("The transformation options");
    this->appendRow(list);

    MetaTreeItem *itemXTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateX, QString::number(0));
    itemXTrans->setEditable(true);
    connect(itemXTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onSurfaceTranslationXChanged);
    list.clear();
    list << itemXTrans;
    list << new QStandardItem(itemXTrans->toolTip());
    pItemTransformationOptions->appendRow(list);

    MetaTreeItem *itemYTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateY, QString::number(0));
    itemYTrans->setEditable(true);
    connect(itemYTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onSurfaceTranslationYChanged);
    list.clear();
    list << itemYTrans;
    list << new QStandardItem(itemYTrans->toolTip());
    pItemTransformationOptions->appendRow(list);

    MetaTreeItem *itemZTrans = new MetaTreeItem(MetaTreeItemTypes::SurfaceTranslateZ, QString::number(0));
    itemZTrans->setEditable(true);
    connect(itemZTrans, &MetaTreeItem::dataChanged,
            this, &Abstract3DTreeItem::onSurfaceTranslationZChanged);
    list.clear();
    list << itemZTrans;
    list << new QStandardItem(itemZTrans->toolTip());
    pItemTransformationOptions->appendRow(list);

     //Do the connects
     connect(this, &Abstract3DTreeItem::checkStateChanged,
             this, &Abstract3DTreeItem::onCheckStateChanged);
}


//*************************************************************************************************************

void Abstract3DTreeItem::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);

    switch(role) {
        case Qt::CheckStateRole:{
            emit checkStateChanged(this->checkState());
            break;
        }
    }
}


//*************************************************************************************************************

int Abstract3DTreeItem::type() const
{
    return m_iType;
}


//*************************************************************************************************************

QList<QStandardItem*> Abstract3DTreeItem::findChildren(int type)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->type() == type) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

QList<QStandardItem*> Abstract3DTreeItem::findChildren(const QString& text)
{
    QList<QStandardItem*> itemList;

    if(this->hasChildren()) {
        for(int row = 0; row<this->rowCount(); row++) {
            for(int col = 0; col<this->columnCount(); col++) {
                if(this->child(row, col)->text() == text) {
                    itemList.append(this->child(row, col));
                }
            }
        }
    }

    return itemList;
}


//*************************************************************************************************************

Abstract3DTreeItem& Abstract3DTreeItem::operator<<(Abstract3DTreeItem* newItem)
{
    this->appendRow(newItem);

    return *this;
}


//*************************************************************************************************************

Abstract3DTreeItem& Abstract3DTreeItem::operator<<(Abstract3DTreeItem& newItem)
{
    this->appendRow(&newItem);

    return *this;
}


//*************************************************************************************************************

void Abstract3DTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    for(int i = 0; i<this->rowCount(); i++) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }

    this->setVisible(checkState == Qt::Unchecked ? false : true);
}


//*************************************************************************************************************

void Abstract3DTreeItem::onSurfaceTranslationXChanged(const QVariant& fTransX)
{
    QVector3D position = this->position();
    position.setX(fTransX.toFloat());
    this->setPosition(position);
}


//*************************************************************************************************************

void Abstract3DTreeItem::onSurfaceTranslationYChanged(const QVariant& fTransY)
{
    QVector3D position = this->position();
    position.setY(fTransY.toFloat());
    this->setPosition(position);
}


//*************************************************************************************************************

void Abstract3DTreeItem::onSurfaceTranslationZChanged(const QVariant& fTransZ)
{
    QVector3D position = this->position();
    position.setZ(fTransZ.toFloat());
    this->setPosition(position);
}
