//=============================================================================================================
/**
 * @file     fieldline_view.cpp
 * @author   Juan Garcia-Prieto <jgarciaprieto@mgh.harvard.edu>
 *           Gabriel Motta <gbmotta@mgh.harvard.edu>
 * @since    0.1.9
 * @date     February, 2023
 *
 * @section  LICENSE
 *
 * Copyright (C) 2023, Gabriel Motta, Juan Garcia-Prieto. All rights reserved.
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
 * @brief     FieldlineView class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "fieldline/fieldline.h"
#include "fieldline/fieldline_view.h"
#include "fieldline/fieldline_view_chassis.h"
#include "formfiles/ui_fieldline_view.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTableWidget>
#include <QLineEdit>
#include <QDebug>
#include <QStringList>

#include <string>
#include <iostream>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


namespace FIELDLINEPLUGIN {

FieldlineView::FieldlineView(Fieldline *parent)
: m_pFieldlinePlugin(parent),
  m_pUi(new Ui::uiFieldlineView)
{
    m_pUi->setupUi(this);
    initTopMenu();
    initAcqSystem(2);
}

FieldlineView::~FieldlineView()
{
    delete m_pUi;
}

void FieldlineView::initTopMenu()
{
    m_pUi->topMenuFrame->setEnabled(true);
    m_pUi->numChassisSpinBox->setMinimum(0);
    m_pUi->numChassisSpinBox->setMaximum(6);
    m_pUi->numChassisSpinBox->setValue(0);
    m_pMacIpTable = new QTableWidget(this);
    m_pMacIpTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_pMacIpTable->verticalHeader()->setSectionHidden(0, true);

    m_pMacIpTable->setColumnCount(2);
    m_pMacIpTable->setHorizontalHeaderLabels(QStringList({"Mac", "IP"}));

    QVBoxLayout* macIpTableLayout = qobject_cast<QVBoxLayout*>(m_pUi->ipMacFrame->layout());
    macIpTableLayout->insertWidget(0, m_pMacIpTable);

    QObject::connect(m_pMacIpTable, QOverload<int, int>::of(&QTableWidget::cellDoubleClicked),
                     this, &FieldlineView::macIpTableDoubleClicked);
    QObject::connect(m_pUi->numChassisSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                     this, &FieldlineView::setNumRowsIpMacFrame);
    QObject::connect(m_pUi->disconnectBtn, &QPushButton::clicked,
                     this, &FieldlineView::disconnect);
}

void FieldlineView::macIpTableDoubleClicked(int row, int col) {
    if ( col == 0 ) {
        std::cout << "cell (" << row << "," << col << ") "
                  << " doubleclicked : " << m_pMacIpTable->item(row, col)->text().toStdString() << "\n";
        std::cout.flush();
        auto callback = [this, row, col](const std::string str) {
            std::cout << "inside callback! :" << str << " \n";
            m_pMacIpTable->item(row, col+1)->setText(str.c_str());
            m_pMacIpTable->repaint();
        };
        m_pFieldlinePlugin->findIpAsync(m_pMacIpTable->item(row, col)->text().toStdString(), callback);
    }
}

void FieldlineView::setNumRowsIpMacFrame(int numRows)
{
    m_pMacIpTable->setRowCount(numRows);
    if (numRows > 0) {
        m_pMacIpTable->setSortingEnabled(false);
        m_pMacIpTable->setItem(numRows-1, 0, new QTableWidgetItem("AF:70:04:21:2D:28"));
        m_pMacIpTable->setItem(numRows-1, 1, new QTableWidgetItem("0.0.0.0"));
        m_pMacIpTable->item(numRows-1, 0)->setToolTip((QString("Doubleclick to find the IP.")));
        m_pMacIpTable->setSortingEnabled(true);
    }
    //    QVBoxLayout* vertIpMacLayout = qobject_cast<QVBoxLayout*>(m_pUi->ipMacFrame->layout());
//    if ( i < m_ipMacList.size())
//    {
//        vertIpMacLayout->removeItem(m_ipMacList.back());
//        delete m_ipMacList.back();
//        m_ipMacList.pop_back();
//    }
//    if ( i > m_ipMacList.size())
//    {
//        QHBoxLayout* ipMacLayout = new QHBoxLayout(m_pUi->ipMacFrame);
//        QLineEdit* ip = new QLineEdit("0.0.0.0");
//        ip->setEnabled(false);
//        QLineEdit* macAddr = new QLineEdit("AF:70:04:21:2D:28");
//        ipMacLayout->addWidget(macAddr);
//        ipMacLayout->addWidget(ip);
//        vertIpMacLayout->insertLayout(m_ipMacList.size() + 1, ipMacLayout);
//        m_ipMacList.push_back(ipMacLayout);
//    }
}

void FieldlineView::disconnect()
{
    qDebug() << "Find chasssis!\n";
    std::cout.flush();

    //generate list of mac addresses
    //call class finder.
    //    retrieve list of ips and set variable with it.
}

void FieldlineView::initAcqSystemTopButtons()
{
    QHBoxLayout* acqSystemTopBtnMenuLayout = qobject_cast<QHBoxLayout*>(m_pUi->acqSystemTopButtonsFrame->layout());

    QPushButton* button = new QPushButton(QString("Start"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::startAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(0, button);

    button = new QPushButton(QString("Stop"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::stopAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(1, button);

    button = new QPushButton(QString("Auto-Tune"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::autoTuneAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(3, button);

    button = new QPushButton(QString("Restart Sensors"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::restartAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(4, button);

    button = new QPushButton(QString("Coarse Zero"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::coarseZeroAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(5, button);

    button = new QPushButton(QString("Fine Zero"), m_pUi->acqSystemTopButtonsFrame);
    QObject::connect(button, &QPushButton::clicked, this, &FieldlineView::fineZeroAllSensors);
    acqSystemTopBtnMenuLayout->insertWidget(6, button);
}

void FieldlineView::initAcqSystem(int numChassis)
{
    // QHBoxLayout* acqSystemTopBtnMenuLayout = qobject_cast<QHBoxLayout*>(m_pUi->acqSystemTopButtonsFrame->layout());
    initAcqSystemTopButtons();
    
    QVBoxLayout* acqSystemRackLayout = qobject_cast<QVBoxLayout*>(m_pUi->chassisRackFrame->layout());

    for( int i = 0; i < numChassis; i++ ) {
        FieldlineViewChassis* pChassis = new FieldlineViewChassis(this, i);
        acqSystemRackLayout->insertWidget(i, pChassis);
        //lkajdsflkasjdf
        m_pAcqSystem.push_back(pChassis);
    }
}


// void FieldlineView::initAcqSystem()
// {
//    QVBoxLayout* rackFrameLayout = qobject_cast<QVBoxLayout*>(m_pUi->fieldlineRackFrame->layout());
//    for (int i = 0; i < numChassis; i++)
//    {
//      FieldlineChassis* chassis = new FieldlineViewChassis(chans[i]);
//      rackFrameLayout->addWidget(chassis);
//    }
// }

void FieldlineView::startAllSensors() {
    std::cout << "startAllSensors" << "\n";
    std::cout.flush();
}

void FieldlineView::stopAllSensors() {
    std::cout << "stopAllSensors" << "\n";
    std::cout.flush();
}

void FieldlineView::autoTuneAllSensors() {
    std::cout << "autoTuneAllSensors" << "\n";
    std::cout.flush();
}

void FieldlineView::restartAllSensors() {
    std::cout << "restartAllSensors" << "\n";
    std::cout.flush();
}

void FieldlineView::coarseZeroAllSensors() {
    std::cout << "coarseZeroAllSensors" << "\n";
    std::cout.flush();
}

void FieldlineView::fineZeroAllSensors() {
    std::cout << "fineZeroAllSensors" << "\n";
    std::cout.flush();
}


//void FieldlineView::setChannelState(size_t chassis_i, size_t chan_i)
//{
//}

//statish FieldlineView::getChannelState(size_t chassis_i, size_t chan_i)
//{
//}

//statish FieldlineView::setAllChannelState(size_t chassis_i, statish)
//{
//}

}  // namespace FIELDLINEPLUGIN

//
// //=============================================================================================================
//
//
// void FieldlineView::clear()
// {
//     for(auto* c : chassis){
//         ui->frame->layout()->removeWidget(c);
//         c->deleteLater();
//     }
// };
//
// //=============================================================================================================
//
// void FieldlineView::setColor(size_t chassis_id, size_t chan_num, const QColor& color)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setColor(chan_num, color);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setColor(size_t chassis_id, size_t chan_num, const QColor& color, bool blinking)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setColor(chan_num, color, blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setChassisColor(size_t chassis_id, const QColor& color)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setColor(color);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setChassisColor(size_t chassis_id, const QColor& color, bool blinking)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setColor(color, blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setAllColor(const QColor& color)
// {
//     for(auto* c : chassis){
//         c->setColor(color);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineView::setAllColor(const QColor& color, bool blinking)
// {
//     for(auto* c : chassis){
//         c->setColor(color, blinking);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineView::setBlinkState(size_t chassis_id, size_t chan_num, bool blinking)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setBlinkState(chan_num, blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setChassisBlinkState(size_t chassis_id, bool blinking)
// {
//     if(chassis_id >= chassis.size()){
//         return;
//     }
//     chassis.at(chassis_id)->setBlinkState(blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineView::setAllBlinkState(bool blinking)
// {
//     for(auto* c : chassis){
//         c->setBlinkState(blinking);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineView::setDefaultNumchans(int num_chans)
// {
//     default_num_chans = num_chans;
// }
