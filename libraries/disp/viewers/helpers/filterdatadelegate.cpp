//=============================================================================================================
/**
* @file     filterdatadelegate.cpp
* @author   Lorenz Esch <Lorenz.Esch@tu-ilmenau.de>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     April, 2015
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
* @brief    Definition of the FilterDataDelegate Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "filterdatadelegate.h"

#include "filterdatamodel.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FilterDataDelegate::FilterDataDelegate(QObject *parent)
: QItemDelegate (parent)
{

}


//*************************************************************************************************************

void FilterDataDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    switch(index.column()) {
        case 0: { //filter state
            drawCheck(painter, option, option.rect, index.data().toBool() ? Qt::Checked : Qt::Unchecked);
            break;
        }
        case 1: { //filter names
            drawDisplay(painter, option, option.rect, index.model()->data(index, Qt::DisplayRole).toString());
            break;
        }
        default:
            QItemDelegate::paint(painter, option, index);
            break;
    }
}

//*************************************************************************************************************

QWidget *FilterDataDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem & option , const QModelIndex & index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);

    if(index.column() == 0) {
        BooleanWidget* widget = new BooleanWidget(parent);

        return widget;
    }

    return 0;
}


//*************************************************************************************************************

void FilterDataDelegate::setEditorData(QWidget *checkBox, const QModelIndex &index) const
{
    if(index.column() != 0)
        return;

    bool value = index.model()->data(index, Qt::DisplayRole).toBool();

    BooleanWidget *checkBoxState = static_cast<BooleanWidget*>(checkBox);

    checkBoxState->setChecked(value);
}


//*************************************************************************************************************

void FilterDataDelegate::setModelData(QWidget *checkBox, QAbstractItemModel *model, const QModelIndex &index) const
{
    if(index.column() != 0)
        return;

    BooleanWidget *checkBoxState = static_cast<BooleanWidget*>(checkBox);

    bool value = checkBoxState->isChecked();

    model->setData(index, value, Qt::EditRole);
}


//*************************************************************************************************************

void FilterDataDelegate::updateEditorGeometry(QWidget *checkBox, const QStyleOptionViewItem &option, const QModelIndex & index) const
{
    Q_UNUSED(index);

    if(index.column() != 0)
        return;

    checkBox->setGeometry(option.rect);
}
