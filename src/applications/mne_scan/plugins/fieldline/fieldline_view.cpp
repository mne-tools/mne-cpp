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
  m_pUi(new Ui::uiFieldlineView),
  m_pAcqSystem(nullptr)
{
    m_pUi->setupUi(this);
    initTopMenu();
    initTopMenuCallbacks();
}

FieldlineView::~FieldlineView()
{
    delete m_pUi;
}

void FieldlineView::initTopMenu()
{
    QVBoxLayout* frameLayout = qobject_cast<QVBoxLayout*>(m_pUi->ipMacFrame->layout());

    QHBoxLayout* ipMacLayout1 = new QHBoxLayout(m_pUi->ipMacFrame);
    QLineEdit* ip1 = new QLineEdit("ip1");
    ip1->setEnabled(false);
    QLineEdit* macAddr1 = new QLineEdit("macaddr1");
    ipMacLayout1->addWidget(macAddr1);
    ipMacLayout1->addWidget(ip1);

    QHBoxLayout* ipMacLayout2 = new QHBoxLayout(m_pUi->ipMacFrame);
    QLineEdit* ip2 = new QLineEdit("ip2");
    ip2->setEnabled(false);
    QLineEdit* macAddr2 = new QLineEdit("macaddr2");
    ipMacLayout2->addWidget(macAddr2);
    ipMacLayout2->addWidget(ip2);

    frameLayout->insertLayout(1, ipMacLayout1);
    frameLayout->insertLayout(1, ipMacLayout2);
}

void FieldlineView::initCallbacks() 
{
}

void FieldlineView::initAcqSystem(int numChassis, const std::vector<std::vector<int>>& chans)
{
    displayAcqSystem();
    initAcqSystemCallbacks();
}

void FieldlineView::displayAcqSystem() 
{
    QVBoxLayout* rackFrameLayout = qobject_cast<QVBxLayout*>(m_pUi->fieldlineRackFrame->layout());
    for (int i = 0; i < numChassis; i++) 
    {
      FieldlineChassis* chassis = new FieldlineViewChassis(chans[i]);

      rackFrameLayout->addWidget(chassis);
    }
}

void FieldlineView::setChannelState(size_t chassis_i, size_t chan_i)
{
}

statish FieldlineView::getChannelState(size_t chassis_i, size_t chan_i)
{
}

statish FieldlineView::setAllChannelState(size_t chassis_i, statish)
{
}

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
