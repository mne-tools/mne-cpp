//=============================================================================================================
/**
 * @file     fieldline_view_chassis.cpp
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

#include <vector>
// #include "fieldline/fieldline.h"
#include "fieldline/fieldline_view.h"
#include "fieldline/fieldline_view_chassis.h"
#include "fieldline/fieldline_view_sensor.h"
#include "formfiles/ui_fieldline_view_chassis.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================
//
// #include <QWidget>
// #include <QLabel>
// #include <QVBoxLayout>
// #include <QMouseEvent>
// #include <QDebug>
// #include <QMenu>

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

//=============================================================================================================

FieldlineViewChassis::FieldlineViewChassis(FieldlineView *parent, int num )
: QWidget(parent),
  m_pFieldlineView(parent),
  m_pUi(new Ui::uiFieldlineViewChassis),
  chassisNum(num),
  numSensors(16)
  {
    m_pUi->setupUi(this);

    std::string chassisName("Fieldline Chassis ");
    chassisName += std::to_string(chassisNum);
    m_pUi->chassisName->setText(QString::fromStdString(chassisName));
    QHBoxLayout* sensorLayout = qobject_cast<QHBoxLayout*>(m_pUi->sensorFrame->layout());

    for (int i = 0; i < numSensors; i++) {
        FieldlineViewSensor* pSensor = new FieldlineViewSensor(this, i);
        sensorLayout->insertWidget(i, pSensor);
        m_pSensors.push_back(pSensor);
    }
}

FieldlineViewChassis::~FieldlineViewChassis()
{
    delete m_pUi;
}

// FieldlineViewChassis::initChannels(size_t numChans)
// {
//     // for(int i = 0; i < num_chans; ++i){
//     //     chans.push_back(new LEDIndicator());
//     //     auto& last_item = chans.back();
//     //     chans.back()->setLabel(QString::number(i + 1));
//     //     ui->chan_frame->layout()->addWidget(chans.back());
//     //     connect(chans.back(), &QWidget::customContextMenuRequested, [this, i, &last_item](const QPoint& pos){this->emit clicked(i, last_item->mapToGlobal(pos)); qDebug() << "clicked " << i+1;});
//     //     connect(this, &FieldlineViewChassis::clicked, this, &FieldlineViewChassis::rightClickMenu, Qt::UniqueConnection);
//     // }
// }

//=============================================================================================================
//
// void FieldlineViewChassis::setColor(size_t chan_num, const QColor& color)
// {
//     if(chan_num > chans.size() || chan_num < 1){
//         return;
//     }
//     chans.at(chan_num - 1)->setColor(color);
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::setColor(size_t chan_num, const QColor& color, bool blinking)
// {
//     setColor(chan_num, color);
//     setBlinkState(chan_num, blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::setColor(const QColor& color)
// {
//     for(auto* chan : chans){
//         chan->setColor(color);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::setColor(const QColor& color, bool blinking)
// {
//     for(auto* chan : chans){
//         chan->setColor(color);
//         chan->setBlink(blinking);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::setBlinkState(size_t chan_num, bool blinking)
// {
//     if(chan_num > chans.size() || chan_num < 1){
//         return;
//     }
//     chans.at(chan_num - 1)->setBlink(blinking);
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::setBlinkState(bool blinking)
// {
//     for(auto* chan : chans){
//         chan->setBlink(blinking);
//     }
// }
//
// //=============================================================================================================
//
// void FieldlineViewChassis::rightClickMenu(int chan, const QPoint& pos)
// {
// //    auto* menu = new QMenu();
//
// //    auto blink_on_chan = menu->addAction("Blink ON - " + QString::number(chan));
// //    auto blink_on_chassis = menu->addAction("Blink ON - Whole Chassis");
//
// //    auto blink_off_chan = menu->addAction("Blink OFF -  " + QString::number(chan));
// //    auto blink_off_chassis = menu->addAction("Blink OFF - Whole Chassis");
//
// //    connect(blink_on_chan, &QAction::triggered,[this, chan](){this->setBlinkState(chan, true);});
// //    connect(blink_off_chan, &QAction::triggered,[this, chan](){this->setBlinkState(chan, false);});
//
// //    connect(blink_on_chassis, &QAction::triggered,[this, chan](){this->setBlinkState(true);});
// //    connect(blink_off_chassis, &QAction::triggered,[this, chan](){this->setBlinkState(false);});
//
// //    menu->exec(pos);
// } 

}  // namespace FIELDLINEPLUGIN

