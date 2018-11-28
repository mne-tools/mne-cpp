//=============================================================================================================
/**
* @file     averageselectionview.cpp
* @author   Lorenz Esch <lesch@mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     July, 2018
*
* @section  LICENSE
*
* Copyright (C) 2018, Lorenz Esch and Matti Hamalainen. All rights reserved.
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
* @brief    Definition of the AverageSelectionView Class.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "averageselectionview.h"

#include <fiff/fiff_evoked_set.h>


//*************************************************************************************************************
//=============================================================================================================
// Qt INCLUDES
//=============================================================================================================

#include <QGridLayout>
#include <QCheckBox>
#include <QColorDialog>
#include <QPalette>
#include <QPushButton>
#include <QDebug>
#include <QPointer>


//*************************************************************************************************************
//=============================================================================================================
// Eigen INCLUDES
//=============================================================================================================

#include<Eigen/Core>


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;
using namespace Eigen;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSelectionView::AverageSelectionView(QWidget *parent,
                                           Qt::WindowFlags f)
: QWidget(parent, f)
, m_iMaxNumAverages(10)
{
    this->setWindowTitle("Average Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void AverageSelectionView::setEvokedSet(QSharedPointer<FIFFLIB::FiffEvokedSet> pEvokedSet)
{

    if(!pEvokedSet) {
        return;
    }

    qDebug() << "AverageSelectionView::setEvokedSet";
    m_pEvokedSet = pEvokedSet;

    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());
    if(!topLayout) {
       topLayout = new QGridLayout();
    }

    QLayoutItem *child;
    while ((child = topLayout->takeAt(0)) != 0) {
        delete child->widget();
        delete child;
    }

    // Create new GUI elements
    for(int i = 0; i < pEvokedSet->evoked.size(); ++i) {
        if(i >= m_iMaxNumAverages) {
            break;
        }

        //Create average active checkbox
        QPointer<QCheckBox> pCheckBox = new QCheckBox(pEvokedSet->evoked.at(i).comment);
        pCheckBox->setChecked(pEvokedSet->evoked.at(i).active);
        pCheckBox->setObjectName(pEvokedSet->evoked.at(i).comment);
        topLayout->addWidget(pCheckBox, i, 0);
        connect(pCheckBox, &QCheckBox::clicked,
                this, &AverageSelectionView::onAveragesChanged);

        //Create average color pushbutton
        Vector4i color = pEvokedSet->evoked.at(i).color;
        QPointer<QPushButton> pButton = new QPushButton();
        pButton->setObjectName(pEvokedSet->evoked.at(i).comment);
        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color[0]).arg(color[1]).arg(color[2]));
        topLayout->addWidget(pButton, i, 1);
        connect(pButton, &QPushButton::clicked,
                this, &AverageSelectionView::onAveragesChanged);
    }

    this->setLayout(topLayout);
}


//*************************************************************************************************************

void AverageSelectionView::onAveragesChanged()
{
    //Change color for average
    if(QPointer<QPushButton> button = qobject_cast<QPushButton*>(sender())) {
        QString sObjectName = button->objectName();

        QColor color = QColorDialog::getColor(button->palette().color(QPalette::Background), this, "Set average color");

        if(button) {
            QPalette palette(QPalette::Button,color);
            button->setPalette(palette);
            button->update();

            //Set color of button new new scene color
            button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        }

        for(int i = 0; i < m_pEvokedSet->evoked.size(); ++i) {
            if(sObjectName == m_pEvokedSet->evoked.at(i).comment) {
                m_pEvokedSet->evoked[i].color = Vector4i(color.red(), color.green(), color.blue(), color.alpha());
            }
        }

        emit averageColorchanged();
    }

    //Change color for average
    if(QPointer<QCheckBox> checkBox = qobject_cast<QCheckBox*>(sender())) {
        QString sObjectName = checkBox->objectName();

        for(int i = 0; i < m_pEvokedSet->evoked.size(); ++i) {
            if(sObjectName == m_pEvokedSet->evoked.at(i).comment) {
                m_pEvokedSet->evoked[i].active = checkBox->isChecked();
            }
        }

        emit averageActivationChanged();
    }
}
