//=============================================================================================================
/**
* @file     data3Dtreedelegate.cpp
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
* @brief    Data3DTreeDelegate class definition.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "data3Dtreedelegate.h"
#include "data3Dtreemodel.h"

#include <disp/imagesc.h>


//*************************************************************************************************************
//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace DISP3DLIB;
using namespace DISPCHARTSLIB;
using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Data3DTreeDelegate::Data3DTreeDelegate(QObject* parent)
: QItemDelegate(parent)
{
}


//*************************************************************************************************************

QWidget *Data3DTreeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option , const QModelIndex& index) const
{
    const Data3DTreeModel* pData3DTreeModel = static_cast<const Data3DTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pData3DTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case MetaTreeItemTypes::SurfaceColorGyri: {
            QColorDialog *pColorDialog = new QColorDialog(parent);
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Gyri Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::SurfaceColorSulci: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Sulci Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::RTDataColormapType: {
            QComboBox* pComboBox = new QComboBox(parent);
            connect(pComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pComboBox->addItem("Hot Negative 1");
            pComboBox->addItem("Hot Negative 2");
            pComboBox->addItem("Hot");
            return pComboBox;
        }

        case MetaTreeItemTypes::RTDataNormalizationValue: {
            Spline* pSpline = new Spline("Spline Histogram", 0);
            connect(pSpline, static_cast<void (Spline::*)(double, double, double)>(&Spline::borderChanged),
            this, &Data3DTreeDelegate::onEditorEdited);
            QStandardItem* pParentItem = static_cast<QStandardItem*>(pAbstractItem->QStandardItem::parent());
            QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);
            MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::RTData).value<MatrixXd>();
            Eigen::VectorXd resultClassLimit;
            Eigen::VectorXi resultFrequency;
            MNEMath::histcounts(matRTData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);
            pSpline->setData(resultClassLimit, resultFrequency, 0);
            QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::RTDataNormalizationValue).value<QVector3D>();
            pSpline->setThreshold(vecThresholdValues);
            AbstractTreeItem* pParentItemAbstract = static_cast<AbstractTreeItem*>(pParentItem);
            QList<QStandardItem*> pColormapItem = pParentItemAbstract->findChildren(MetaTreeItemTypes::RTDataColormapType);
            for(int i = 0; i < pColormapItem.size(); ++i)
            {
                QModelIndex indexColormapItem = pData3DTreeModel->indexFromItem(pColormapItem.at(i));
                QString colorMap = index.model()->data(indexColormapItem, MetaTreeItemRoles::RTDataColormapType).value<QString>();
                pSpline->setColorMap(colorMap);
            }
            return pSpline;
        }

        case MetaTreeItemTypes::RTDataTimeInterval: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setSuffix(" mSec");
            pSpinBox->setMinimum(17);
            pSpinBox->setMaximum(50000);
            pSpinBox->setSingleStep(10);
            pSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::RTDataTimeInterval).toInt());
            return pSpinBox;
        }

        case MetaTreeItemTypes::RTDataVisualizationType: {
            QComboBox* pComboBox = new QComboBox(parent);
            pComboBox->addItem("Vertex based");
            pComboBox->addItem("Smoothing based");
            pComboBox->addItem("Annotation based");
            return pComboBox;
        }

        case MetaTreeItemTypes::SurfaceColor: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Surface Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::PointColor: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Point Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::RTDataNumberAverages: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(100);
            pSpinBox->setSingleStep(1);
            pSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::RTDataNumberAverages).toInt());
            return pSpinBox;
        }

        case MetaTreeItemTypes::SurfaceAlpha: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceAlpha).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTranslateX: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateX).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTranslateY: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateY).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTranslateZ: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateZ).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::NetworkThreshold: {
            Spline* pSpline = new Spline("Spline Histogram", 0);
            connect(pSpline, static_cast<void (Spline::*)(double, double, double)>(&Spline::borderChanged),
                this, &Data3DTreeDelegate::onEditorEdited);

            QStandardItem* pParentItem = static_cast<QStandardItem*>(pAbstractItem->QStandardItem::parent());
            QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);
            MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::NetworkDataMatrix).value<MatrixXd>();
            Eigen::VectorXd resultClassLimit;
            Eigen::VectorXi resultFrequency;
            MNEMath::histcounts(matRTData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);
            pSpline->setData(resultClassLimit, resultFrequency, 0);
            QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::NetworkThreshold).value<QVector3D>();
            pSpline->setThreshold(vecThresholdValues);

            return pSpline;
        }

        case MetaTreeItemTypes::NetworkMatrix: {
            QStandardItem* pParentItem = static_cast<QStandardItem*>(pAbstractItem->QStandardItem::parent());
            QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);
            MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::NetworkDataMatrix).value<MatrixXd>();

            ImageSc* pPlotLA = new ImageSc(matRTData);
            pPlotLA->show();
            //return pPlotLA;
        }

        default: // do nothing;
            break;
    }


    return QItemDelegate::createEditor(parent, option, index);
}


//*************************************************************************************************************

void Data3DTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    const Data3DTreeModel* pData3DTreeModel = static_cast<const Data3DTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pData3DTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case MetaTreeItemTypes::SurfaceColorGyri: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::SurfaceColorGyri).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::SurfaceColorSulci: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::SurfaceColorSulci).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::RTDataColormapType: {
            QString colormap = index.model()->data(index, MetaTreeItemRoles::RTDataColormapType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(colormap);
            break;
        }

        case MetaTreeItemTypes::RTDataNormalizationValue: {
            Spline* pSpline = static_cast<Spline*>(editor);
            int width = pSpline->size().width();
            int height = pSpline->size().height();
            if (pSpline->size().width() < 200 && pSpline->size().height() < 200)
            {
                pSpline->resize(600,400);   //resize histogram to be readable with default size
            }
            else
            {
                width = pSpline->size().width();   //keeps the size of the histogram
                height = pSpline->size().height();
                pSpline->resize(width,height);
            }
            return;
        }

        case MetaTreeItemTypes::RTDataTimeInterval: {
            int value = index.model()->data(index, MetaTreeItemRoles::RTDataTimeInterval).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::RTDataVisualizationType: {
            QString visType = index.model()->data(index, MetaTreeItemRoles::RTDataVisualizationType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(visType);
            break;
        }

        case MetaTreeItemTypes::SurfaceColor: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::SurfaceColor).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::PointColor: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::PointColor).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::RTDataNumberAverages: {
            int value = index.model()->data(index, MetaTreeItemRoles::RTDataNumberAverages).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceAlpha: {
            int value = index.model()->data(index, MetaTreeItemRoles::SurfaceAlpha).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateX: {
            double value = index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateX).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateY: {
            double value = index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateY).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateZ: {
            double value = index.model()->data(index, MetaTreeItemRoles::SurfaceTranslateZ).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }        

        case MetaTreeItemTypes::NetworkThreshold: {
            Spline* pSpline = static_cast<Spline*>(editor);
            int width = pSpline->size().width();
            int height = pSpline->size().height();
            if (pSpline->size().width() < 200 && pSpline->size().height() < 200)   //pSpline initializes with size (137,15)
            {
                pSpline->resize(800,600);   //resize histogram to be readable with default size
            }
            else
            {
                width = pSpline->size().width();   //keeps the size of the histogram
                height = pSpline->size().height();
                pSpline->resize(width,height);
            }
            return;
        }

        default: // do nothing;
            break;
    }

    QItemDelegate::setEditorData(editor, index);
}


//*************************************************************************************************************

void Data3DTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    const Data3DTreeModel* pData3DTreeModel = static_cast<const Data3DTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pData3DTreeModel->itemFromIndex(index));

    switch(pAbstractItem->type()) {
        case MetaTreeItemTypes::SurfaceColorGyri: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::SurfaceColorGyri);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case MetaTreeItemTypes::SurfaceColorSulci: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::SurfaceColorSulci);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case MetaTreeItemTypes::RTDataColormapType: {
            QComboBox* pColorMapType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pColorMapType->currentText());

            model->setData(index, data, MetaTreeItemRoles::RTDataColormapType);
            model->setData(index, data, Qt::DisplayRole);
            return;
        }

        case MetaTreeItemTypes::RTDataNormalizationValue: {
            Spline* pSpline = static_cast<Spline*>(editor);
            QVector3D returnVector;
            returnVector = pSpline->getThreshold();

            QString displayThreshold;
            displayThreshold = QString("%1,%2,%3").arg(returnVector.x()).arg(returnVector.y()).arg(returnVector.z());
            QVariant dataDisplay;
            dataDisplay.setValue(displayThreshold);
            model->setData(index, dataDisplay, Qt::DisplayRole);

            model->setData(index, returnVector, MetaTreeItemRoles::RTDataNormalizationValue);
            return;
        }

        case MetaTreeItemTypes::RTDataTimeInterval: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::RTDataTimeInterval);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::RTDataVisualizationType: {
            QComboBox* pVisType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pVisType->currentText());

            model->setData(index, data, MetaTreeItemRoles::RTDataVisualizationType);
            model->setData(index, data, Qt::DisplayRole);
            return;
        }

        case MetaTreeItemTypes::SurfaceColor: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::SurfaceColor);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case MetaTreeItemTypes::PointColor: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::PointColor);
            model->setData(index, data, Qt::DecorationRole);
            return;
        }

        case MetaTreeItemTypes::RTDataNumberAverages: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::RTDataNumberAverages);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceAlpha: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceAlpha);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateX: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTranslateX);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateY: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTranslateY);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTranslateZ: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTranslateZ);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::NetworkThreshold: {
            Spline* pSpline = static_cast<Spline*>(editor);
            QVector3D returnVector;
            returnVector = pSpline->getThreshold();

            QString displayThreshold;
            displayThreshold = QString("%1,%2,%3").arg(returnVector.x()).arg(returnVector.y()).arg(returnVector.z());
            QVariant dataDisplay;
            dataDisplay.setValue(displayThreshold);
            model->setData(index, dataDisplay, Qt::DisplayRole);

            model->setData(index, returnVector, MetaTreeItemRoles::NetworkThreshold);
            return;
        }

        default: // do nothing;
            break;
    }

    QItemDelegate::setModelData(editor, model, index);
}


//*************************************************************************************************************

void Data3DTreeDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}


//*************************************************************************************************************

void Data3DTreeDelegate::onEditorEdited()
{
    QWidget* editor = qobject_cast<QWidget*>(QObject::sender());
    emit commitData(editor);
}



