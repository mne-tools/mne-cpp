//=============================================================================================================
/**
* @file     braintreedelegate.cpp
* @author   Lorenz Esch <lorenz.esch@tu-ilmenau.de>;
*           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
*           Jens Haueisen <jens.haueisen@tu-ilmenau.de>
* @version  1.0
* @date     August, 2014
*
* @section  LICENSE
*
* Copyright (C) 2014, Lorenz Esch, Christoph Dinh, Matti Hamalainen and Jens Haueisen. All rights reserved.
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
* @brief    BrainTreeDelegate class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "braintreedelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISP3DNEWLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeDelegate::BrainTreeDelegate(QObject *parent)
: QItemDelegate(parent)
{
}


//*************************************************************************************************************

QWidget *BrainTreeDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option , const QModelIndex &index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
    case BrainTreeModelItemTypes::SurfaceColorGyri: {
        QColorDialog *pColorDialog = new QColorDialog(parent);
        pColorDialog->show();
        return pColorDialog;
    }

    case BrainTreeModelItemTypes::SurfaceColorSulci: {
        QColorDialog *pColorDialog = new QColorDialog(parent);
        pColorDialog->show();
        return pColorDialog;
    }

    case BrainTreeModelItemTypes::SurfaceColorInfoOrigin: {
        QComboBox* pComboBox = new QComboBox(parent);
        pComboBox->addItem("Color from curvature");
        pComboBox->addItem("Color from annotation");
        pComboBox->addItem("Color from RT source loc");
        return pComboBox;
    }
    }

    return QItemDelegate::createEditor(parent, option, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
    case BrainTreeModelItemTypes::SurfaceColorGyri: {
        QColor color = index.model()->data(index, BrainTreeItemRoles::SurfaceColorGyri).value<QColor>();
        QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
        pColorDialog->setCurrentColor(color);
        break;
    }

    case BrainTreeModelItemTypes::SurfaceColorSulci: {
        QColor color = index.model()->data(index, BrainTreeItemRoles::SurfaceColorSulci).value<QColor>();
        QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
        pColorDialog->setCurrentColor(color);
        break;
    }

    case BrainTreeModelItemTypes::SurfaceColorInfoOrigin: {
        QString colorOrigin = index.model()->data(index, BrainTreeItemRoles::SurfaceColorInfoOrigin).toString();
        QComboBox* pComboBox = static_cast<QComboBox*>(editor);
        pComboBox->setCurrentText(colorOrigin);
        break;
    }
    }

    QItemDelegate::setEditorData(editor, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
    case BrainTreeModelItemTypes::SurfaceColorGyri: {
        QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
        QColor color = pColorDialog->currentColor();
        QVariant data;
        data.setValue(color);

        model->setData(index, data, BrainTreeItemRoles::SurfaceColorGyri);
        model->setData(index, data, Qt::DecorationRole);
        return;
    }

    case BrainTreeModelItemTypes::SurfaceColorSulci: {
        QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
        QColor color = pColorDialog->currentColor();
        QVariant data;
        data.setValue(color);

        model->setData(index, data, BrainTreeItemRoles::SurfaceColorSulci);
        model->setData(index, data, Qt::DecorationRole);
        return;
    }

    case BrainTreeModelItemTypes::SurfaceColorInfoOrigin: {
        QComboBox* pColorDialog = static_cast<QComboBox*>(editor);
        QVariant data;
        data.setValue(pColorDialog->currentText());

        model->setData(index, data, BrainTreeItemRoles::SurfaceColorInfoOrigin);
        model->setData(index, data, Qt::DisplayRole);
        return;
    }

    }

    QItemDelegate::setModelData(editor, model, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
