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

#include "fieldline/fieldline_view.h"
#include "fieldline/fieldline.h"
#include "formfiles/ui_fieldline_view.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QVBoxLayout>
#include <QLineEdit>

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

FieldlineView::FieldlineView(Fieldline* parent)
: m_pFieldlinePlugin(parent),
  m_pUi(new Ui::uiFieldlineView)
//  m_pAcqSystem(nullptr)
{
    m_pUi->setupUi(this);
    initTopMenu();
//    initTopMenuCallbacks();
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
    QObject::connect(m_pUi->numChassisSpinBox,QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FieldlineView::setNumRowsIpMacFrame);
    QObject::connect(m_pUi->findBtn, &QPushButton::clicked,
                     this, &FieldlineView::findChassis);
}

void FieldlineView::setNumRowsIpMacFrame(int i)
{
    QVBoxLayout* frameLayout = qobject_cast<QVBoxLayout*>(m_pUi->ipMacFrame->layout());
    if ( i < m_ipMacList.size())
    {
        frameLayout->removeWidget(qobject_cast<QWidget*>(m_ipMacList.back()));
        m_ipMacList.pop_back();
    }
    if ( i > m_ipMacList.size())
    {
        QHBoxLayout* ipMacLayout = new QHBoxLayout(m_pUi->ipMacFrame);
        QLineEdit* ip = new QLineEdit("0.0.0.0");
        ip->setEnabled(false);
        QLineEdit* macAddr = new QLineEdit("AF-70-04-21-2D-28");
        ipMacLayout->addWidget(macAddr);
        ipMacLayout->addWidget(ip);
        frameLayout->insertLayout(m_ipMacList.size() + 1, ipMacLayout);
        m_ipMacList.push_back(ipMacLayout);
    }
}

void FieldlineView::findChassis()
{
    //generate list of mac addresses
    //call class finder.
    //    retrieve list of ips and set variable with it.
}
void FieldlineView::initCallbacks() 
{
}

void FieldlineView::initAcqSystem(int numChassis, const std::vector<std::vector<int>>& chans)
{
    displayAcqSystem();
}

void FieldlineView::displayAcqSystem() 
{
//    QVBoxLayout* rackFrameLayout = qobject_cast<QVBoxLayout*>(m_pUi->fieldlineRackFrame->layout());
//    for (int i = 0; i < numChassis; i++)
//    {
//      FieldlineChassis* chassis = new FieldlineViewChassis(chans[i]);

//      rackFrameLayout->addWidget(chassis);
//    }
}


//void FieldlineView::on_numChassisSpinBox_valueChanged(int arg1)
//{

//}

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
