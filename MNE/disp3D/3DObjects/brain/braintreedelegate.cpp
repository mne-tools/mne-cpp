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

using namespace DISP3DLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

BrainTreeDelegate::BrainTreeDelegate(QObject* parent)
: QItemDelegate(parent)
{
}


//*************************************************************************************************************

QWidget *BrainTreeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option , const QModelIndex& index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case BrainTreeMetaItemTypes::SurfaceColorGyri: {
            QColorDialog *pColorDialog = new QColorDialog(parent);
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &BrainTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Gyri Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case BrainTreeMetaItemTypes::SurfaceColorSulci: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &BrainTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Sulci Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case BrainTreeMetaItemTypes::RTDataColormapType: {
            QComboBox* pComboBox = new QComboBox(parent);
            connect(pComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &BrainTreeDelegate::onEditorEdited);
            pComboBox->addItem("Hot Negative 1");
            pComboBox->addItem("Hot Negative 2");
            pComboBox->addItem("Hot");
            return pComboBox;
        }

        case BrainTreeMetaItemTypes::RTDataNormalizationValue: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &BrainTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.0001);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, BrainTreeMetaItemRoles::RTDataNormalizationValue).toDouble());
            return pDoubleSpinBox;
            break;
        }

        case BrainTreeMetaItemTypes::RTDataTimeInterval: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &BrainTreeDelegate::onEditorEdited);
            pSpinBox->setSuffix(" mSec");
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(5000);
            pSpinBox->setSingleStep(10);
            pSpinBox->setValue(index.model()->data(index, BrainTreeMetaItemRoles::RTDataTimeInterval).toInt());
            return pSpinBox;
            break;
        }

        case BrainTreeMetaItemTypes::RTDataVisualizationType: {
            QComboBox* pComboBox = new QComboBox(parent);
            pComboBox->addItem("Vertex based");
            pComboBox->addItem("Smoothing based");
            pComboBox->addItem("Annotation based");
            return pComboBox;
        }

        case BrainTreeMetaItemTypes::SurfaceColorItem: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &BrainTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Surface Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case BrainTreeMetaItemTypes::RTDataNumberAverages: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &BrainTreeDelegate::onEditorEdited);
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(100);
            pSpinBox->setSingleStep(1);
            pSpinBox->setValue(index.model()->data(index, BrainTreeMetaItemRoles::RTDataNumberAverages).toInt());
            return pSpinBox;
            break;
        }

        case BrainTreeMetaItemTypes::SurfaceAlpha: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &BrainTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, BrainTreeMetaItemRoles::SurfaceAlpha).toDouble());
            return pDoubleSpinBox;
            break;
        }
    }


    return QItemDelegate::createEditor(parent, option, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case BrainTreeMetaItemTypes::SurfaceColorGyri: {
            QColor color = index.model()->data(index, BrainTreeMetaItemRoles::SurfaceColorGyri).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case BrainTreeMetaItemTypes::SurfaceColorSulci: {
            QColor color = index.model()->data(index, BrainTreeMetaItemRoles::SurfaceColorSulci).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataColormapType: {
            QString colormap = index.model()->data(index, BrainTreeMetaItemRoles::RTDataColormapType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(colormap);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataNormalizationValue: {
            double value = index.model()->data(index, BrainTreeMetaItemRoles::RTDataNormalizationValue).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataTimeInterval: {
            int value = index.model()->data(index, BrainTreeMetaItemRoles::RTDataTimeInterval).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataVisualizationType: {
            QString visType = index.model()->data(index, BrainTreeMetaItemRoles::RTDataVisualizationType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(visType);
            break;
        }

        case BrainTreeMetaItemTypes::SurfaceColorItem: {
            QColor color = index.model()->data(index, BrainTreeMetaItemRoles::SurfaceColor).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataNumberAverages: {
            int value = index.model()->data(index, BrainTreeMetaItemRoles::RTDataNumberAverages).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case BrainTreeMetaItemTypes::SurfaceAlpha: {
            int value = index.model()->data(index, BrainTreeMetaItemRoles::SurfaceAlpha).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }
    }

    QItemDelegate::setEditorData(editor, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    const BrainTreeModel* pBrainTreeModel = static_cast<const BrainTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pBrainTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case BrainTreeMetaItemTypes::SurfaceColorGyri: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, BrainTreeMetaItemRoles::SurfaceColorGyri);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case BrainTreeMetaItemTypes::SurfaceColorSulci: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, BrainTreeMetaItemRoles::SurfaceColorSulci);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case BrainTreeMetaItemTypes::RTDataColormapType: {
            QComboBox* pColorMapType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pColorMapType->currentText());

            model->setData(index, data, BrainTreeMetaItemRoles::RTDataColormapType);
            model->setData(index, data, Qt::DisplayRole);
            return;
        }

        case BrainTreeMetaItemTypes::RTDataNormalizationValue: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, BrainTreeMetaItemRoles::RTDataNormalizationValue);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataTimeInterval: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, BrainTreeMetaItemRoles::RTDataTimeInterval);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case BrainTreeMetaItemTypes::RTDataVisualizationType: {
            QComboBox* pVisType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pVisType->currentText());

            model->setData(index, data, BrainTreeMetaItemRoles::RTDataVisualizationType);
            model->setData(index, data, Qt::DisplayRole);
            return;
        }

        case BrainTreeMetaItemTypes::SurfaceColorItem: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, BrainTreeMetaItemRoles::SurfaceColor);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case BrainTreeMetaItemTypes::RTDataNumberAverages: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, BrainTreeMetaItemRoles::RTDataNumberAverages);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case BrainTreeMetaItemTypes::SurfaceAlpha: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, BrainTreeMetaItemRoles::SurfaceAlpha);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }
    }

    QItemDelegate::setModelData(editor, model, index);
}


//*************************************************************************************************************

void BrainTreeDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


//*************************************************************************************************************

void BrainTreeDelegate::onEditorEdited()
{
    QWidget* editor = qobject_cast<QWidget*>(QObject::sender());
    emit commitData(editor);
}



