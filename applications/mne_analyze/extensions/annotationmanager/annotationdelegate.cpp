//=============================================================================================================
/**
 * @file     annotationdelegate.h
 * @author   Lorenz Esch <lesch@mgh.harvard.edu>;
 *           Gabriel Motta <gbmotta@mgh.harvard.edu
 * @since    0.1.0
 * @date     April, 2020
 *
 * @section  LICENSE
 *
 * Copyright (C) 2020, Lorenz Esch, Gabriel Motta. All rights reserved.
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
 * @brief    Contains the definition of the annotationdelegate class.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "annotationdelegate.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QDebug>
#include <QSettings>
#include <QEvent>

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AnnotationDelegate::AnnotationDelegate(QObject *parent)
: QItemDelegate(parent)
{

}

//=============================================================================================================

QWidget* AnnotationDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex & index) const
{
    const ANSHAREDLIB::AnnotationModel* pAnnotationModel = static_cast<const ANSHAREDLIB::AnnotationModel*>(index.model());

    switch(index.column()) {
        case 0: {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setMinimum(0);
            editor->setMaximum(pAnnotationModel->getFirstLastSample().second);
            connect(editor, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, &AnnotationDelegate::onSampleValueChanged);
            return editor;
        }

        case 1: {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setMinimum(0.00);
            editor->setMaximum(pAnnotationModel->getFirstLastSample().second / pAnnotationModel->getSampleFreq());
            editor->setSingleStep(0.020);
            connect(editor, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AnnotationDelegate::onTimeValueChanged);
            return editor;
        }

        case 2: {
            QComboBox *editor = new QComboBox(parent);
            editor->addItems(pAnnotationModel->getEventTypeList());
            return editor;
        }
    }

    QWidget *returnWidget = new QWidget();
    return returnWidget;
}

//=============================================================================================================

void AnnotationDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    qDebug() << "AnnotationDelegate::setEditorData";
    switch(index.column()) {
        case 0: {
            int value = index.model()->data(index, Qt::DisplayRole).toInt();
            QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
            spinBox->setValue(value);
            break;
        }

        case 1: {
            double value = index.model()->data(index, Qt::DisplayRole).toDouble();
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
            spinBox->setValue(value);
            break;
        }

        case 2: {
            int value = index.model()->data(index, Qt::DisplayRole).toInt();
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            spinBox->setCurrentText(QString().number(value));
            break;
        }
    }
}

//=============================================================================================================

void AnnotationDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    qDebug() << "setModelData";
    switch(index.column()) {
        case 0: {
            QSpinBox *spinBox = static_cast<QSpinBox*>(editor);
            spinBox->interpretText();
            int value = spinBox->value();

            model->setData(index, value, Qt::EditRole);
            break;
        }

        case 1: {
            QDoubleSpinBox *spinBox = static_cast<QDoubleSpinBox*>(editor);
            spinBox->interpretText();
            double value = spinBox->value();

            model->setData(index, value, Qt::EditRole);
            break;
        }

        case 2: {
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            QString value = spinBox->currentText();

            model->setData(index, value.toInt(), Qt::EditRole);
            break;
        }
    }
}

//=============================================================================================================

void AnnotationDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}

//=============================================================================================================

void AnnotationDelegate::onSampleValueChanged(int iValue)
{
    qDebug() << "[AnnotationDelegate::onSampleValueChanged] -- " << iValue ;
    emit sampleValueChanged(iValue);
}

//=============================================================================================================

void AnnotationDelegate::onTimeValueChanged(double dValue)
{
    qDebug() << "[AnnotationDelegate::onTimeValueChanged] -- " << dValue ;
    emit timeValueChanged(dValue);
}

//=============================================================================================================

//bool AnnotationDelegate::eventFilter(QObject *object, QEvent *event)
//{
//    qDebug() << event->type();
//    return true;
//}
