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


//*************************************************************************************************************
//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using namespace DISPLIB;


//*************************************************************************************************************
//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

AverageSelectionView::AverageSelectionView(QWidget *parent,
                         Qt::WindowFlags f)
: QWidget(parent, f)
{
    this->setWindowTitle("Average Selection");
    this->setMinimumWidth(330);
    this->setMaximumWidth(330);
}


//*************************************************************************************************************

void AverageSelectionView::init()
{
//    //Delete all widgets in the averages layout
//    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());

//    if(!topLayout) {
//       topLayout = new QGridLayout();
//    }

//    QLayoutItem *child;
//    bool bKeepItem = false;

//    while ((child = topLayout->takeAt(0)) != 0) {
//        if(child->widget()->objectName() == QString(sAvrType)) {
//            bKeepItem = true;
//            break;
//        }
//    }



//    QMapIterator<double, AverageSelectionInfo> i(m_qMapAverageInfo);
//    int count = 0;

//    while (i.hasNext() && count < 10) {
//        i.next();

//        QString sAvrType = i.value().second.first;


//        while ((child = topLayout->takeAt(0)) != 0) {
//            if(child->widget()->objectName() == QString(sAvrType)) {
//                bKeepItem = true;
//                break;
//            }
//        }

//        if(!bKeepItem) {
//            delete child->widget();
//            delete child;
//        }
//    }







//    //Create trigger type check boxes. Limit to 10.
//    QMapIterator<double, AverageSelectionInfo> i(m_qMapAverageInfo);
//    int count = 0;
//    m_qMapButtonAverageType.clear();
//    m_qMapChkBoxAverageType.clear();

//    while (i.hasNext() && count < 10) {
//        i.next();

//        //Create average checkbox
//        QCheckBox* pCheckBox = new QCheckBox(i.value().second.first);
//        pCheckBox->setChecked(i.value().second.second);
//        topLayout->addWidget(pCheckBox, count, 0);
//        connect(pCheckBox, &QCheckBox::clicked,
//                this, &AverageSelectionView::onAveragesChanged);
//        m_qMapChkBoxAverageType.insert(pCheckBox, i.value().second.first.toDouble());

//        //Create average color pushbutton
//        QPushButton* pButton = new QPushButton();
//        pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(i.value().first.red()).arg(i.value().first.green()).arg(i.value().first.blue()));
//        topLayout->addWidget(pButton, count, 1);
//        connect(pButton, &QPushButton::clicked,
//                this, &AverageSelectionView::onAveragesChanged);
//        m_qMapButtonAverageType.insert(pButton, i.value().second.first.toDouble());

//        ++count;
//    }

//    //Find Filter tab and add current layout
//    this->setLayout(topLayout);
}


//*************************************************************************************************************

void AverageSelectionView::setAverageInformationMapOld(const QMap<double, AverageSelectionInfo>& qMapAverageInfoOld)
{
    m_qMapAverageInfoOld = qMapAverageInfoOld;
}


//*************************************************************************************************************

void AverageSelectionView::setAverageInformationMap(const QMap<double, AverageSelectionInfo> &qMapAverageSelectionInfo)
{
    //m_qMapAverageInfo = qMapAverageSelectionInfo;
    //Delete all widgets in the averages layout
    QGridLayout* topLayout = static_cast<QGridLayout*>(this->layout());

    if(!topLayout) {
       topLayout = new QGridLayout();
       this->setLayout(topLayout);
    }

    //Check if average type already exists in the map
    QMapIterator<double, AverageSelectionInfo> i(qMapAverageSelectionInfo);

    while (i.hasNext()) {
        i.next();

        if(!m_qMapAverageInfo.contains(i.key())) {
            m_qMapAverageInfo.insert(i.key(), i.value());

            int row = topLayout->count();

            // Create checkbox
            QPointer<QCheckBox> pCheckBox = new QCheckBox(i.value().name);
            pCheckBox->setObjectName(i.value().name);
            pCheckBox->setChecked(i.value().active);
            topLayout->addWidget(pCheckBox, row, 0);
            connect(pCheckBox.data(), &QCheckBox::clicked,
                    this, &AverageSelectionView::onAveragesChanged);
            m_qMapChkBoxAverageType.insert(pCheckBox, i.value().name.toDouble());

            // Create pushbutton
            QPointer<QPushButton> pButton = new QPushButton();
            pButton->setObjectName(i.value().name);
            pButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(i.value().color.red()).arg(i.value().color.green()).arg(i.value().color.blue()));
            topLayout->addWidget(pButton, row, 1);
            connect(pButton.data(), &QPushButton::clicked,
                    this, &AverageSelectionView::onAveragesChanged);
            m_qMapButtonAverageType.insert(pButton, i.value().name.toDouble());
        }

//        if(m_qMapAverageInfoOld.contains(i.key())) {
//            //Use old color
//            AverageSelectionInfo tempPair = i.value();
//            tempPair.first = m_qMapAverageInfoOld[i.key()].first;

//            m_qMapAverageInfo.insert(i.key(), tempPair);
//        } else {
//            //Use default color
//            m_qMapAverageInfo.insert(i.key(), i.value());
//        }
    }

    QLayoutItem *child;
    bool bKeepItem = false;
    int count = topLayout->count();

    for(int i = 0; i < count; ++i) {
        if(!m_qMapAverageInfo.contains(topLayout->itemAt(i)->widget()->objectName())) {
            topLayout->
        }
    }

    while ((child = topLayout->itemAt(0)) != 0) {
        if(child->widget()->objectName() == QString(sAvrType)) {
            bKeepItem = true;
            break;
        }
    }

    //Recreate average group
    //init();

    emit averageInformationChanged(m_qMapAverageInfo);
}


//*************************************************************************************************************

QMap<double, AverageSelectionInfo> AverageSelectionView::getAverageInformationMap()
{
    return m_qMapAverageInfo;
}


//*************************************************************************************************************

void AverageSelectionView::onAveragesChanged()
{
    //Change color for average
    if(QPointer<QPushButton> button = qobject_cast<QPushButton*>(sender())) {
        QColor color = QColorDialog::getColor(m_qMapAverageInfo[m_qMapButtonAverageType[button]].color, this, "Set average color");

        if(button) {
            QPalette palette(QPalette::Button,color);
            button->setPalette(palette);
            button->update();

            //Set color of button new new scene color
            button->setStyleSheet(QString("background-color: rgb(%1, %2, %3);").arg(color.red()).arg(color.green()).arg(color.blue()));

            m_qMapAverageInfo[m_qMapButtonAverageType[button]].color = color;

            emit averageInformationChanged(m_qMapAverageInfo);
        }
    }

    //Change color for average
    if(QPointer<QCheckBox> checkBox = qobject_cast<QCheckBox*>(sender())) {
        m_qMapAverageInfo[m_qMapChkBoxAverageType[checkBox]].active = checkBox->isChecked();

        emit averageInformationChanged(m_qMapAverageInfo);
    }
}
