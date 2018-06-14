//=============================================================================================================
/**
* @file     eventdelegate.cpp
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
* @brief    Definition of delegate of EventDelegate
*
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

    QWidget *returnWidget = new QWidget();
    return returnWidget;
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
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            spinBox->setCurrentText(QString().number(value));
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
            QComboBox *spinBox = static_cast<QComboBox*>(editor);
            QString value = spinBox->currentText();

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
