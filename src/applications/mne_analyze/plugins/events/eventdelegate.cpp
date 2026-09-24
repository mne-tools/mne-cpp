//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2021-2026 MNE-CPP Authors
 *
 * @file     eventdelegate.cpp
 * @author   Gabriel Motta <gabrielbenmotta@gmail.com>;
 *           Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @since    0.1.9
 * @date     April, 2021
 * @brief    Contains the definition of the eventdelegate class.
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventdelegate.h"

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

EventDelegate::EventDelegate(QObject *parent)
: QItemDelegate(parent)
{

}

//=============================================================================================================

QWidget* EventDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex & index) const
{
    const ANSHAREDLIB::EventModel* pEventModel = static_cast<const ANSHAREDLIB::EventModel*>(index.model());

    switch(index.column()) {
        case 0: {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setMinimum(0);
            editor->setMaximum(pEventModel->getFirstLastSample().second);
            return editor;
        }

        case 1: {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setMinimum(0.00);
            editor->setMaximum(pEventModel->getFirstLastSample().second / pEventModel->getSampleFreq());
            editor->setSingleStep(0.100);
            return editor;
        }
    }

    QWidget *returnWidget = new QWidget();
    return returnWidget;
}

//=============================================================================================================

void EventDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
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
    }
}

//=============================================================================================================

void EventDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
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
    }
}

//=============================================================================================================

void EventDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
