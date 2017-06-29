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

        case MetaTreeItemTypes::ColormapType: {
            QComboBox* pComboBox = new QComboBox(parent);
            connect(pComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pComboBox->addItem("Hot Negative 1");
            pComboBox->addItem("Hot Negative 2");
            pComboBox->addItem("Hot");
            pComboBox->addItem("Jet");
            return pComboBox;
        }

        case MetaTreeItemTypes::DistributedSourceLocThreshold: {
            Spline* pSpline = new Spline("Set Threshold", 0);
            connect(pSpline, static_cast<void (Spline::*)(double, double, double)>(&Spline::borderChanged),
            this, &Data3DTreeDelegate::onEditorEdited);

            //Find the parent and retreive rt source loc data to calcualte the histogram
            if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);

                //Get data
                MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::RTData).value<MatrixXd>();

                //Calcualte histogram
                Eigen::VectorXd resultClassLimit;
                Eigen::VectorXi resultFrequency;
                MNEMath::histcounts(matRTData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);

                //Create histogram plot
                pSpline->setData(resultClassLimit, resultFrequency, 0);
                QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::DistributedSourceLocThreshold).value<QVector3D>();
                pSpline->setThreshold(vecThresholdValues);

                QList<QStandardItem*> pColormapItem = pParentItem->findChildren(MetaTreeItemTypes::ColormapType);
                for(int i = 0; i < pColormapItem.size(); ++i) {
                    QModelIndex indexColormapItem = pData3DTreeModel->indexFromItem(pColormapItem.at(i));
                    QString colorMap = index.model()->data(indexColormapItem, MetaTreeItemRoles::ColormapType).value<QString>();
                    pSpline->setColorMap(colorMap);
                }
            }

            return pSpline;
        }

        case MetaTreeItemTypes::StreamingTimeInterval: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setSuffix(" mSec");
            pSpinBox->setMinimum(0);
            pSpinBox->setMaximum(50000);
            pSpinBox->setSingleStep(1);
            pSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::StreamingTimeInterval).toInt());
            return pSpinBox;
        }

        case MetaTreeItemTypes::VisualizationType: {
            QComboBox* pComboBox = new QComboBox(parent);
            pComboBox->addItem("Vertex based");
            pComboBox->addItem("Smoothing based");
            pComboBox->addItem("Annotation based");
            return pComboBox;
        }

        case MetaTreeItemTypes::Color: {
            QColorDialog *pColorDialog = new QColorDialog();
            connect(pColorDialog, &QColorDialog::currentColorChanged,
                    this, &Data3DTreeDelegate::onEditorEdited);
            pColorDialog->setWindowTitle("Select Color");
            pColorDialog->show();
            pColorDialog->setCurrentColor(index.model()->data(index, MetaTreeItemRoles::Color).value<QColor>());
            return pColorDialog;
        }

        case MetaTreeItemTypes::NumberAverages: {
            QSpinBox* pSpinBox = new QSpinBox(parent);
            connect(pSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pSpinBox->setMinimum(1);
            pSpinBox->setMaximum(100);
            pSpinBox->setSingleStep(1);
            pSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::NumberAverages).toInt());
            return pSpinBox;
        }

        case MetaTreeItemTypes::AlphaValue: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::AlphaValue).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTessInner: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(1.0);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(1.0);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTessInner).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTessOuter: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(1.0);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(1.0);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTessOuter).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::SurfaceTriangleScale: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.01);
            pDoubleSpinBox->setMaximum(100.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::SurfaceTriangleScale).toDouble());
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
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::TranslateX).toDouble());
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
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::TranslateY).toDouble());
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
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::TranslateZ).toDouble());
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
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::Scale).toDouble());
            return pDoubleSpinBox;
        }

        case MetaTreeItemTypes::NetworkThreshold: {
            Spline* pSpline = new Spline("Set Threshold", 0);
            connect(pSpline, static_cast<void (Spline::*)(double, double, double)>(&Spline::borderChanged),
                this, &Data3DTreeDelegate::onEditorEdited);

            //Find the parent and retreive network data to calcualte the histogram
            if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);

                //Get data
                MatrixXd matNetworkData = index.model()->data(indexParent, Data3DTreeModelItemRoles::NetworkDataMatrix).value<MatrixXd>().array().abs();

                //Calcualte histogram
                Eigen::VectorXd resultClassLimit;
                Eigen::VectorXi resultFrequency;
                MNEMath::histcounts(matNetworkData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);

                //Create histogram plot
                pSpline->setData(resultClassLimit, resultFrequency, 0);
                QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::NetworkThreshold).value<QVector3D>();
                pSpline->setThreshold(vecThresholdValues);
            }

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

        case MetaTreeItemTypes::MaterialType: {
            QComboBox* pComboBox = new QComboBox(parent);

            pComboBox->setCurrentText(index.model()->data(index, MetaTreeItemRoles::SurfaceMaterial).toString());
            pComboBox->addItem("Phong Alpha Tesselation");
            pComboBox->addItem("Phong Alpha");
            return pComboBox;
        }
        
        case MetaTreeItemTypes::CancelDistance: {
            QDoubleSpinBox* pDoubleSpinBox = new QDoubleSpinBox(parent);
            connect(pDoubleSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
                    this, &Data3DTreeDelegate::onEditorEdited);
            pDoubleSpinBox->setMinimum(0.001);
            pDoubleSpinBox->setMaximum(1.0);
            pDoubleSpinBox->setSingleStep(0.01);
            pDoubleSpinBox->setSuffix("m");
            pDoubleSpinBox->setValue(index.model()->data(index, MetaTreeItemRoles::CancelDistance).toDouble());
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

        case MetaTreeItemTypes::ColormapType: {
            QString colormap = index.model()->data(index, MetaTreeItemRoles::ColormapType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(colormap);
            break;
        }

        case MetaTreeItemTypes::DistributedSourceLocThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                //Find the parent and retreive rt source loc data to calcualte the histogram
                if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                    QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);

                    //Get data
                    MatrixXd matRTData = index.model()->data(indexParent, Data3DTreeModelItemRoles::RTData).value<MatrixXd>();

                    //Calcualte histogram
                    Eigen::VectorXd resultClassLimit;
                    Eigen::VectorXi resultFrequency;
                    MNEMath::histcounts(matRTData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);

                    //Create histogram plot
                    pSpline->setData(resultClassLimit, resultFrequency, 0);
                    QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::DistributedSourceLocThreshold).value<QVector3D>();
                    pSpline->setThreshold(vecThresholdValues);

                    QList<QStandardItem*> pColormapItem = pParentItem->findChildren(MetaTreeItemTypes::ColormapType);
                    for(int i = 0; i < pColormapItem.size(); ++i) {
                        QModelIndex indexColormapItem = pData3DTreeModel->indexFromItem(pColormapItem.at(i));
                        QString colorMap = index.model()->data(indexColormapItem, MetaTreeItemRoles::ColormapType).value<QString>();
                        pSpline->setColorMap(colorMap);
                    }
                }

                int width = pSpline->size().width();
                int height = pSpline->size().height();
                if (pSpline->size().width() < 200 && pSpline->size().height() < 200) {
                    pSpline->resize(600,400);   //resize histogram to be readable with default size
                } else {
                    width = pSpline->size().width();   //keeps the size of the histogram
                    height = pSpline->size().height();
                    pSpline->resize(width,height);
                }
            }

            return;
        }

        case MetaTreeItemTypes::StreamingTimeInterval: {
            int value = index.model()->data(index, MetaTreeItemRoles::StreamingTimeInterval).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::VisualizationType: {
            QString visType = index.model()->data(index, MetaTreeItemRoles::VisualizationType).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(visType);
            break;
        }

        case MetaTreeItemTypes::Color: {
            QColor color = index.model()->data(index, MetaTreeItemRoles::Color).value<QColor>();
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            pColorDialog->setCurrentColor(color);
            break;
        }

        case MetaTreeItemTypes::NumberAverages: {
            int value = index.model()->data(index, MetaTreeItemRoles::NumberAverages).toInt();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::AlphaValue: {
            int value = index.model()->data(index, MetaTreeItemRoles::AlphaValue).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTessInner: {
            int value = index.model()->data(index, MetaTreeItemRoles::SurfaceTessInner).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTessOuter: {
            int value = index.model()->data(index, MetaTreeItemRoles::SurfaceTessOuter).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::SurfaceTriangleScale: {
            int value = index.model()->data(index, MetaTreeItemRoles::SurfaceTriangleScale).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::TranslateX: {
            double value = index.model()->data(index, MetaTreeItemRoles::TranslateX).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::TranslateY: {
            double value = index.model()->data(index, MetaTreeItemRoles::TranslateY).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::TranslateZ: {
            double value = index.model()->data(index, MetaTreeItemRoles::TranslateZ).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::Scale: {
            double value = index.model()->data(index, MetaTreeItemRoles::Scale).toDouble();
            QDoubleSpinBox* pDoubleSpinBox = static_cast<QDoubleSpinBox*>(editor);
            pDoubleSpinBox->setValue(value);
            break;
        }

        case MetaTreeItemTypes::NetworkThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                //Find the parent and retreive network data to calcualte the histogram
                if(AbstractTreeItem* pParentItem = static_cast<AbstractTreeItem*>(pAbstractItem->QStandardItem::parent())) {
                    QModelIndex indexParent = pData3DTreeModel->indexFromItem(pParentItem);

                    //Get data
                    MatrixXd matNetworkData = index.model()->data(indexParent, Data3DTreeModelItemRoles::NetworkDataMatrix).value<MatrixXd>().array().abs();

                    //Calcualte histogram
                    Eigen::VectorXd resultClassLimit;
                    Eigen::VectorXi resultFrequency;
                    MNEMath::histcounts(matNetworkData, false, 50, resultClassLimit, resultFrequency, 0.0, 0.0);

                    //Create histogram plot
                    pSpline->setData(resultClassLimit, resultFrequency, 0);
                    QVector3D vecThresholdValues = index.model()->data(index, MetaTreeItemRoles::NetworkThreshold).value<QVector3D>();
                    pSpline->setThreshold(vecThresholdValues);
                }

                int width = pSpline->size().width();
                int height = pSpline->size().height();
                //pSpline initializes with size (137,15)
                if (pSpline->size().width() < 200 && pSpline->size().height() < 200) {
                    pSpline->resize(800,600);   //resize histogram to be readable with default size
                } else {
                    width = pSpline->size().width();   //keeps the size of the histogram
                    height = pSpline->size().height();
                    pSpline->resize(width,height);
                }
            }

            return;
        }

        case MetaTreeItemTypes::MaterialType: {
            QString materialType = index.model()->data(index, MetaTreeItemRoles::SurfaceMaterial).toString();
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            pComboBox->setCurrentText(materialType);
            return;
        }
        case MetaTreeItemTypes::CancelDistance: {
            int value = index.model()->data(index, MetaTreeItemRoles::CancelDistance).toDouble();
            QSpinBox* pSpinBox = static_cast<QSpinBox*>(editor);
            pSpinBox->setValue(value);
            break;
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

        case MetaTreeItemTypes::ColormapType: {
            QComboBox* pColorMapType = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pColorMapType->currentText());

            model->setData(index, data, MetaTreeItemRoles::ColormapType);
            model->setData(index, data, Qt::DisplayRole);
            return;
        }

        case MetaTreeItemTypes::DistributedSourceLocThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                QVector3D returnVector;
                returnVector = pSpline->getThreshold();

                QString displayThreshold;
                displayThreshold = QString("%1,%2,%3").arg(returnVector.x()).arg(returnVector.y()).arg(returnVector.z());
                QVariant data;
                data.setValue(displayThreshold);
                model->setData(index, data, Qt::DisplayRole);
                data.setValue(returnVector);
                model->setData(index, data, MetaTreeItemRoles::DistributedSourceLocThreshold);
            }
            return;
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
            return;
        }

        case MetaTreeItemTypes::Color: {
            QColorDialog* pColorDialog = static_cast<QColorDialog*>(editor);
            QColor color = pColorDialog->currentColor();
            QVariant data;
            data.setValue(color);

            model->setData(index, data, MetaTreeItemRoles::Color);
            model->setData(index, data, Qt::DecorationRole);
            return;
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

        case MetaTreeItemTypes::NetworkThreshold: {
            if(Spline* pSpline = static_cast<Spline*>(editor)) {
                QVector3D returnVector;
                returnVector = pSpline->getThreshold();

                QString displayThreshold;
                displayThreshold = QString("%1,%2,%3").arg(returnVector.x()).arg(returnVector.y()).arg(returnVector.z());

                QVariant data;
                data.setValue(displayThreshold);
                model->setData(index, data, Qt::DisplayRole);
                data.setValue(returnVector);
                model->setData(index, data, MetaTreeItemRoles::NetworkThreshold);
            }

            return;
        }

        case MetaTreeItemTypes::MaterialType: {
            QComboBox* pComboBox = static_cast<QComboBox*>(editor);
            QVariant data;
            data.setValue(pComboBox->currentText());

            model->setData(index, data, MetaTreeItemRoles::SurfaceMaterial);
            model->setData(index, data, Qt::DisplayRole);
            return;
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
    if(QWidget* editor = qobject_cast<QWidget*>(QObject::sender())) {
        emit commitData(editor);
    }
}



