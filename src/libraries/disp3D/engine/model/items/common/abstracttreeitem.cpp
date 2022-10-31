//=============================================================================================================
/**
 * @file     abstracttreeitem.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>
 * @since    0.1.0
 * @date     November, 2015
 *
 * @section  LICENSE
 *
 * Copyright (C) 2015, Lorenz Esch. All rights reserved.
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
 * @brief    AbstractTreeItem class definition.
 *
 */
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "abstracttreeitem.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DLIB;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AbstractTreeItem::AbstractTreeItem(int iType, const QString& text)
: QStandardItem(text)
, m_iType(iType)
{
    initItem();
}

//=============================================================================================================

void AbstractTreeItem::initItem()
{
    this->setToolTip("Abstract Tree Item");

    //Do the connects
    connect(this, &AbstractTreeItem::checkStateChanged,
         this, &AbstractTreeItem::onCheckStateChanged);
}

//=============================================================================================================

void AbstractTreeItem::setData(const QVariant& value, int role)
{
    QStandardItem::setData(value, role);

    switch(role) {
        case Qt::CheckStateRole:{
            emit checkStateChanged(this->checkState());
            break;
        }
    }
}

//=============================================================================================================

int AbstractTreeItem::type() const
{
    return m_iType;
}

//=============================================================================================================

void AbstractTreeItem::addItemWithDescription(QStandardItem* pItemParent,
                                              QStandardItem* pItemAdd)
{
    if(pItemParent && pItemAdd) {
        QList<QStandardItem*> list;
        list << pItemAdd;
        list << new QStandardItem(pItemAdd->toolTip());
        pItemParent->appendRow(list);
    }
}

//=============================================================================================================

QList<QStandardItem*> AbstractTreeItem::findChildren(int type)
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

//=============================================================================================================

QList<QStandardItem*> AbstractTreeItem::findChildren(const QString& text)
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

//=============================================================================================================

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem* newItem)
{
    this->appendRow(newItem);

    return *this;
}

//=============================================================================================================

AbstractTreeItem& AbstractTreeItem::operator<<(AbstractTreeItem& newItem)
{
    this->appendRow(&newItem);

    return *this;
}

//=============================================================================================================

void AbstractTreeItem::onCheckStateChanged(const Qt::CheckState& checkState)
{
    //Store old state
    m_checkStateOld = this->checkState();

    for(int i = 0; i<this->rowCount(); i++) {
        if(this->child(i)->isCheckable()) {
            this->child(i)->setCheckState(checkState);
        }
    }
}
