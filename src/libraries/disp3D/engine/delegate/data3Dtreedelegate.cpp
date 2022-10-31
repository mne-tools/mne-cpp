//=============================================================================================================
/**
 * @file     data3Dtreedelegate.cpp
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
 * @since    0.1.0
 * @date     August, 2014
 *
 * @section  LICENSE
 *
 * Copyright (C) 2014, Lorenz Esch, Christoph Dinh. All rights reserved.
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

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "data3Dtreedelegate.h"
#include "../model/data3Dtreemodel.h"

#include <disp/plots/imagesc.h>
#include <disp/plots/spline.h>

#include <utils/mnemath.h>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace UTILSLIB;
using namespace DISP3DLIB;
using namespace DISPLIB;
using namespace Eigen;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

Data3DTreeDelegate::Data3DTreeDelegate(QObject* parent)
: QStyledItemDelegate(parent)
{
}

//=============================================================================================================

QWidget *Data3DTreeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option , const QModelIndex& index) const
{
    // Only create the editors here. Do not set any data from the model yet. This is done in setEditorData().
    //Connect to Data3DTreeDelegate::onEditorEdited if you want to have immediate feedback (default by QItemDelegate is onEditorFinished).
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
            QColorDialog *pColorDialog = new QColorDialog(parent);
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Sulci Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::ColormapType: {
            QComboBox* pComboBox = new QComboBox(parent);
            connect(pComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pComboBox->addItem("HotNegative1");
            pComboBox->addItem("HotNegative2");
            pComboBox->addItem("Hot");
            pComboBox->addItem("Jet");
            pComboBox->addItem("RedBlue");
            pComboBox->addItem("Bone");
            pComboBox->addItem("Cool");
            pComboBox->addItem("Viridis");
            pComboBox->addItem("ViridisNegated");
            return pComboBox;
        }

        case MetaTreeItemTypes::DataThreshold: {
            Spline* pSpline = new Spline(parent);
            connect(pSpline, static_cast<void (Spline::*)(double, double, double)>(&Spline::borderChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpline->setWindowFlags(Qt::Window);
            pSpline->setWindowTitle("Set Threshold");
            pSpline->show();
            return pSpline;
        }

        case MetaTreeItemTypes::StreamingTimeInterval: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setSuffix(" mSec");
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(50000);
            pSpinBox->setSingleStep(1);
            return pSpinBox;
        }

        case MetaTreeItemTypes::VisualizationType: {
            QComboBox* pComboBox = new QComboBox(parent);
            pComboBox->addItem("Interpolation based");
            pComboBox->addItem("Annotation based");
            return pComboBox;
        }

        case MetaTreeItemTypes::Color: {
            QColorDialog *pColorDialog = new QColorDialog(parent);
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Color");
            pColorDialog->show();
            return pColorDialog;
        }

        case MetaTreeItemTypes::NumberAverages: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(100);
            pSpinBox->setSingleStep(1);
            return pSpinBox;
        }

        case MetaTreeItemTypes::AlphaValue: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTessInner: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(1.0);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(1.0);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTessOuter: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(1.0);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(1.0);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTriangleScale: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)()>(&QDoubleSpinBox::editingFinished),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(0.01);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::TranslateX: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.001);
            pDoubleSpinBox->setDecimals(5);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::TranslateY: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.001);
            pDoubleSpinBox->setDecimals(5);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::TranslateZ: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.001);
            pDoubleSpinBox->setDecimals(5);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::Scale: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(-10000.0);
            pDoubleSpinBox->setMaximum(10000.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setDecimals(3);
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::NetworkMatrix: {
            if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);
                MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::Data).value<MatrixXd>();

                ImageSc* pPlotLA = new ImageSc(matRTData, parent);
                pPlotLA->resize(400,300);
                pPlotLA->setWindowFlags(Qt::Window);
                pPlotLA->show();

                QList<QStandardItem*> pColormapItem = pParentItem->findChildren(MetaTreeItemTypes::ColormapType);
                for(int i = 0; i < pColormapItem.size(); ++i) {
                    if(pColormapItem.at(i)) {
                        QModelIndex indexColormapItem = pData3DTreeModel->indexFromItem(pColormapItem.at(i));
                        QString colorMap = index.model()->data(indexColormapItem, MetaTreeItemRoles::ColormapType).value<QString>();
                        pPlotLA->setColorMap(colorMap);
                    }
                }
            }
            break;
            //return pPlotLA;
        }

        case MetaTreeItemTypes::MaterialType: {
            QComboBox* pComboBox = new QComboBox(parent);
            pComboBox->addItem("Phong Alpha Tesselation");
            pComboBox->addItem("Phong Alpha");
            pComboBox->addItem("Show normals");
            pComboBox->addItem("GPU Interpolation");
            return pComboBox;
        }
        
        case MetaTreeItemTypes::CancelDistance: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            pDoubleSpinBox->setMinimum(0.001);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setSuffix("m");
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::InterpolationFunction: {
            QComboBox* pComboBox = new QComboBox(parent);
            connect(pComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pComboBox->addItem("Linear");
            pComboBox->addItem("Square");
            pComboBox->addItem("Cubic");
            pComboBox->addItem("Gaussian");
            return pComboBox;
        }

        default: {
                break;
        }
    }

    return QStyledItemDelegate::createEditor(parent, option, index);
}

//=============================================================================================================

void Data3DTreeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{    
    const Data3DTreeModel* pData3DTreeModel = static_cast<const Data3DTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pData3DTreeModel->itemFromIndex(index));

    // Only catch non-standard Qt types such as QColorDialog or items which need special handling for display role (e.g. Thresholding) etc.
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

        case MetaTreeItemTypes::Color: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::Color).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::DataThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                //Find the parent and retreive real-time data to calcualte the histogram
                if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                    QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);

                    //Get data
                    MatrixXd matRTData;
                    QVariant data;

                    data = index.model()->data(indexParent, Data3DTreeModelItemRoles::Data);

                    if(data.canConvert<MatrixXd>()) {
                        matRTData = data.value<MatrixXd>();
                        matRTData = matRTData.cwiseAbs();

                        //Calcualte histogram
                        Eigen::VectorXd resultClassLimit;
                        Eigen::VectorXi resultFrequency;
                        MNEMath::histcounts(matRTData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);

                        //Create histogram plot
                        pSpline->setData(resultClassLimit, resultFrequency);
                        QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::DataThreshold).value<QVector3D>();
                        pSpline->setThreshold(vecThresholdValues);

                        QList<QStandardItem*> pColormapItem = pParentItem->findChildren(MetaTreeItemTypes::ColormapType);
                        for(int i = 0; i < pColormapItem.size(); ++i) {
                            if(pColormapItem.at(i)) {
                                QModelIndex indexColormapItem = pData3DTreeModel->indexFromItem(pColormapItem.at(i));
                                QString colorMap = index.model()->data(indexColormapItem, MetaTreeItemRoles::ColormapType).value<QString>();
                                pSpline->setColorMap(colorMap);
                            }
                        }
                    }
                }

                pSpline->resize(600,400);
                pSpline->show();
            }

            break;
        }

        // Handle basic types (QString, int, double, etc.) by default via QItemDelegate::setEditorData
        default: {
            QStyledItemDelegate::setEditorData(editor, index);
        }
    }
}

//=============================================================================================================

void Data3DTreeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    const Data3DTreeModel* pData3DTreeModel = static_cast<const Data3DTreeModel*>(index.model());
    const AbstractTreeItem* pAbstractItem = static_cast<const AbstractTreeItem*>(pData3DTreeModel->itemFromIndex(index));

    //Set data manually here so we can use our own item roles.
    switch(pAbstractItem->type()) {
        case MetaTreeItemTypes::SurfaceColorGyri: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::SurfaceColorGyri);
            model->setData(index, data, Qt::DecorationRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceColorSulci: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::SurfaceColorSulci);
            model->setData(index, data, Qt::DecorationRole);
            break;
        }

        case MetaTreeItemTypes::ColormapType: {
            QComboBox* pColorMapType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pColorMapType->currentText());

            model->setData(index, data, MetaTreeItemRoles::ColormapType);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::DataThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                QVector3D returnVector;
                returnVector = pSpline->getThreshold();

                QString displayThreshold;
                displayThreshold = QString("%1,%2,%3").arg(returnVector.x()).arg(returnVector.y()).arg(returnVector.z());
                QVariant data;
                data.setValue(displayThreshold);
                model->setData(index, data, Qt::DisplayRole);
                data.setValue(returnVector);
                model->setData(index, data, MetaTreeItemRoles::DataThreshold);
            }
            break;
        }

        case MetaTreeItemTypes::StreamingTimeInterval: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::StreamingTimeInterval);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::VisualizationType: {
            QComboBox* pVisType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pVisType->currentText());

            model->setData(index, data, MetaTreeItemRoles::VisualizationType);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::Color: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::Color);
            model->setData(index, data, Qt::DecorationRole);
            break;
        }

        case MetaTreeItemTypes::NumberAverages: {
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);

            QVariant data;
            data.setValue(pSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::NumberAverages);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::AlphaValue: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::AlphaValue);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTessInner: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTessInner);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTessOuter: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTessOuter);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::SurfaceTriangleScale: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::SurfaceTriangleScale);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::TranslateX: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::TranslateX);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::TranslateY: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::TranslateY);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::TranslateZ: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::TranslateZ);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::Scale: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::Scale);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::MaterialType: {
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pComboBox->currentText());

            model->setData(index, data, MetaTreeItemRoles::SurfaceMaterial);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::CancelDistance: {
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            QVariant data;
            data.setValue(pDoubleSpinBox->value());

            model->setData(index, data, MetaTreeItemRoles::CancelDistance);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        case MetaTreeItemTypes::InterpolationFunction: {
            QComboBox* pColorMapType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pColorMapType->currentText());

            model->setData(index, data, MetaTreeItemRoles::InterpolationFunction);
            model->setData(index, data, Qt::DisplayRole);
            break;
        }

        // Handle all other item types via QItemDelegate::setModelData handling
        default: {
            QStyledItemDelegate::setModelData(editor, model, index);
            break;
        }
    }
}

//=============================================================================================================

void Data3DTreeDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem &option, const QModelIndex& index) const
{
    QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    //editor->setGeometry(option.rect);
}

//=============================================================================================================

void Data3DTreeDelegate::onEditorEdited()
{
    if(QWidget* editor = qobject_cast<QWidget*>(QObject::sender())) {
        emit commitData(editor);
    }
}

