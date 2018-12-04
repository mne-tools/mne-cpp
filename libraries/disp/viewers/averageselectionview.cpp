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
, m_qMapAverageColor(QSharedPointer<QMap<QString, QColor> >::create())
, m_qMapAverageActivation(QSharedPointer<QMap<QString, bool> >::create())
{
    this->setWindowTitle("Average Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

QSharedPointer<QMap<QString, QColor> > AverageSelectionView::getAverageColor() const
{
    return m_qMapAverageColor;
}


//*************************************************************************************************************

QSharedPointer<QMap<QString, bool> > AverageSelectionView::getAverageActivation() const
{
    return m_qMapAverageActivation;
}


//*************************************************************************************************************

void AverageSelectionView::setAverageColor(const QSharedPointer<QMap<QString, QColor> > qMapAverageColor)
{
    m_qMapAverageColor = qMapAverageColor;
    update();
}


//*************************************************************************************************************

void AverageSelectionView::setAverageActivation(const QSharedPointer<QMap<QString, bool> > qMapAverageActivation)
{
    m_qMapAverageActivation = qMapAverageActivation;
    update();
}


//*************************************************************************************************************

void AverageSelectionView::update()
{
    if(m_qMapAverageColor->size() != m_qMapAverageActivation->size()) {
        qDebug() << "AverageSelectionView::update - m_qMapAverageColor and m_qMapAverageActivation do not match in size. Returning.";
        return;
    }

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
    QMapIterator<QString, QColor> itr(*m_qMapAverageColor);
    int count = 0;
    while(itr.hasNext()) {
        if(count >= m_iMaxNumAverages) {
            break;
        }

        itr.next();

        //Create average active checkbox
        QPointer<QCheckBox> pCheckBox = new QCheckBox(itr.key());
        pCheckBox->setChecked(m_qMapAverageActivation->value(itr.key()));
        pCheckBox->setObjectName(itr.key());
        topLayout->addWidget(pCheckBox, count, 0);
        connect(pCheckBox.data(), &QCheckBox::clicked,
                this, &AverageSelectionView::onAveragesChanged);

        //Create average color pushbutton
        QColor color = itr.value();
        QPointer<QPushButton> pButton = new QPushButton();
        pButton->setObjectName(itr.key());
        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        topLayout->addWidget(pButton, count, 1);
        connect(pButton.data(), &QPushButton::clicked,
                this, &AverageSelectionView::onAveragesChanged);

        count++;
    }

    this->setLayout(topLayout);
}


//*************************************************************************************************************

void AverageSelectionView::onAveragesChanged()
{
    //Change color for average
    if(QPointer<QPushButton> button = qobject_cast<QPushButton*>(sender())) {
        QString sObjectName = button->objectName();

        QColor color = QColorDialog::getColor(m_qMapAverageColor->value(sObjectName), this, "Set average color");

        if(button) {
            QPalette palette(QPalette::Button,color);
            button->setPalette(palette);
            button->update();

            //Set color of button new new scene color
            button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));
        }

        m_qMapAverageColor->insert(sObjectName, color);

        emit newAverageColorMap(m_qMapAverageColor);
    }

    //Change color for average
    if(QPointer<QCheckBox> checkBox = qobject_cast<QCheckBox*>(sender())) {
        QString sObjectName = checkBox->objectName();

        m_qMapAverageActivation->insert(sObjectName, checkBox->isChecked());

        emit newAverageActivationMap(m_qMapAverageActivation);
    }
}
