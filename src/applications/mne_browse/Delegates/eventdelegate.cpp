//=============================================================================================================
/**
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2014-2026 MNE-CPP Authors
 *
 * @file     eventdelegate.cpp
 * @author   Christoph Dinh <christoph.dinh@mne-cpp.org>;
 *           Lorenz Esch <lorenz.esch@tu-ilmenau.de>
 * @date     August, 2014
 * @version  2.1.0
 * @brief    Definition of delegate of EventDelegate
 */

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "eventdelegate.h"


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace MNEBROWSE;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

EventDelegate::EventDelegate(QObject *parent)
: QItemDelegate(parent)
{
}


//*************************************************************************************************************

QWidget *EventDelegate::createEditor(QWidget *parent,
     const QStyleOptionViewItem &/* option */,
     const QModelIndex & index) const
{
    const EventModel* pEventModel = static_cast<const EventModel*>(index.model());

    switch(index.column()) {
        case 0: {
            QSpinBox *editor = new QSpinBox(parent);
            editor->setMinimum(0);
            editor->setMaximum(pEventModel->getFirstLastSample().second);
            return editor;
        }

        case 1: {
            QDoubleSpinBox *editor = new QDoubleSpinBox(parent);
            editor->setMinimum(0.0);
            editor->setMaximum(pEventModel->getFirstLastSample().second / pEventModel->getFiffInfo()->sfreq);
            editor->setSingleStep(0.01);
            return editor;
        }

        case 2: {
            QComboBox *editor = new QComboBox(parent);
            editor->addItems(pEventModel->getEventTypeList());
            return editor;
        }
    }

    return new QWidget(parent);
}


//*************************************************************************************************************

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

        case 2: {
            int value = index.model()->data(index, Qt::DisplayRole).toInt();
            QComboBox *comboBox = static_cast<QComboBox*>(editor);
            comboBox->setCurrentText(QString().number(value));
            break;
        }
    }
}


//*************************************************************************************************************

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

        case 2: {
            QComboBox *comboBox = static_cast<QComboBox*>(editor);
            QString value = comboBox->currentText();

            model->setData(index, value.toInt(), Qt::EditRole);
            break;
        }
    }
}


//*************************************************************************************************************

void EventDelegate::updateEditorGeometry(QWidget *editor,
    const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
