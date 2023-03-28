//=============================================================================================================
/**
 * @file     fl_rack.cpp
 * @author   Gabriel Motta <gbmotta@mgh.harvard.edu>
 *           Juan Garcia-Prieto <juangpc@gmail.com>
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

#include "ui_fl_rack.h"
#include "ui_fl_chassis.h"

#include "fieldlineview.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QMouseEvent>
#include <QDebug>
#include <QMenu>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using DISPLIB::FieldlineView;
using DISPLIB::fl_chassis;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

int FieldlineView::default_num_sensors = 16;

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================


FieldlineView::FieldlineView(int num_chassis, int sensors_per_chassis, QWidget *parent)
: FieldlineView(parent)
{
    configure(num_chassis, sensors_per_chassis);
}

//=============================================================================================================

FieldlineView::FieldlineView(int num_chassis, QWidget *parent)
: FieldlineView(num_chassis, default_num_sensors, parent)
{
}

//=============================================================================================================

FieldlineView::FieldlineView(QWidget *parent)
: QWidget(parent)
, ui(new Ui::fl_rack())
{
    ui->setupUi(this);
}
//=============================================================================================================

FieldlineView::~FieldlineView()
{
    delete ui;
}

//=============================================================================================================

void FieldlineView::configure(int num_chassis)
{
    configure(num_chassis, default_num_sensors);
}

//=============================================================================================================

void FieldlineView::configure(int num_chassis, int num_sensors)
{
    clear();
    for(int i = 0; i < num_chassis; ++i){
        chassis.push_back(new fl_chassis(num_sensors));
        ui->frame->layout()->addWidget(chassis.back());
    }
}

//=============================================================================================================

void FieldlineView::clear()
{
    for(auto* c : chassis){
        ui->frame->layout()->removeWidget(c);
        c->deleteLater();
    }
};

//=============================================================================================================

void FieldlineView::setColor(size_t chassis_id, size_t sensor_num, const QColor& color)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setColor(sensor_num, color);
}

//=============================================================================================================

void FieldlineView::setColor(size_t chassis_id, size_t sensor_num, const QColor& color, bool blinking)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setColor(sensor_num, color, blinking);
}

//=============================================================================================================

void FieldlineView::setChassisColor(size_t chassis_id, const QColor& color)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setColor(color);
}

//=============================================================================================================

void FieldlineView::setChassisColor(size_t chassis_id, const QColor& color, bool blinking)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setColor(color, blinking);
}

//=============================================================================================================

void FieldlineView::setAllColor(const QColor& color)
{
    for(auto* c : chassis){
        c->setColor(color);
    }
}

//=============================================================================================================

void FieldlineView::setAllColor(const QColor& color, bool blinking)
{
    for(auto* c : chassis){
        c->setColor(color, blinking);
    }
}

//=============================================================================================================

void FieldlineView::setBlinkState(size_t chassis_id, size_t sensor_num, bool blinking)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setBlinkState(sensor_num, blinking);
}

//=============================================================================================================

void FieldlineView::setChassisBlinkState(size_t chassis_id, bool blinking)
{
    if(chassis_id >= chassis.size()){
        return;
    }
    chassis.at(chassis_id)->setBlinkState(blinking);
}

//=============================================================================================================

void FieldlineView::setAllBlinkState(bool blinking)
{
    for(auto* c : chassis){
        c->setBlinkState(blinking);
    }
}

//=============================================================================================================

void FieldlineView::setDefaultNumSensors(int num_sensors)
{
    default_num_sensors = num_sensors;
}

//=============================================================================================================

fl_chassis::fl_chassis(int num_sensors, QWidget *parent )
: QWidget(parent)
, ui(new Ui::fl_chassis())
{
    ui->setupUi(this);

    for(int i = 0; i < num_sensors; ++i){
        sensors.push_back(new LEDIndicator());
        auto& last_item = sensors.back();
        sensors.back()->setLabel(QString::number(i + 1));
        ui->sensor_frame->layout()->addWidget(sensors.back());
        connect(sensors.back(), &QWidget::customContextMenuRequested, [this, i, &last_item](const QPoint& pos){this->emit clicked(i, last_item->mapToGlobal(pos)); qDebug() << "clicked " << i+1;});
        connect(this, &fl_chassis::clicked, this, &fl_chassis::rightClickMenu, Qt::UniqueConnection);
    }
}

//=============================================================================================================

fl_chassis::~fl_chassis()
{
    delete ui;
}

//=============================================================================================================

void fl_chassis::setColor(size_t sensor_num, const QColor& color)
{
    if(sensor_num > sensors.size() || sensor_num < 1){
        return;
    }
    sensors.at(sensor_num - 1)->setColor(color);
}

//=============================================================================================================

void fl_chassis::setColor(size_t sensor_num, const QColor& color, bool blinking)
{
    setColor(sensor_num, color);
    setBlinkState(sensor_num, blinking);
}

//=============================================================================================================

void fl_chassis::setColor(const QColor& color)
{
    for(auto* sensor : sensors){
        sensor->setColor(color);
    }
}

//=============================================================================================================

void fl_chassis::setColor(const QColor& color, bool blinking)
{
    for(auto* sensor : sensors){
        sensor->setColor(color);
        sensor->setBlink(blinking);
    }
}

//=============================================================================================================

void fl_chassis::setBlinkState(size_t sensor_num, bool blinking)
{
    if(sensor_num > sensors.size() || sensor_num < 1){
        return;
    }
    sensors.at(sensor_num - 1)->setBlink(blinking);
}

//=============================================================================================================

void fl_chassis::setBlinkState(bool blinking)
{
    for(auto* sensor : sensors){
        sensor->setBlink(blinking);
    }
}

//=============================================================================================================

void fl_chassis::rightClickMenu(int sensor, const QPoint& pos)
{
//    auto* menu = new QMenu();

//    auto blink_on_sensor = menu->addAction("Blink ON - " + QString::number(sensor));
//    auto blink_on_chassis = menu->addAction("Blink ON - Whole Chassis");

//    auto blink_off_sensor = menu->addAction("Blink OFF -  " + QString::number(sensor));
//    auto blink_off_chassis = menu->addAction("Blink OFF - Whole Chassis");

//    connect(blink_on_sensor, &QAction::triggered,[this, sensor](){this->setBlinkState(sensor, true);});
//    connect(blink_off_sensor, &QAction::triggered,[this, sensor](){this->setBlinkState(sensor, false);});

//    connect(blink_on_chassis, &QAction::triggered,[this, sensor](){this->setBlinkState(true);});
//    connect(blink_off_chassis, &QAction::triggered,[this, sensor](){this->setBlinkState(false);});

//    menu->exec(pos);
}
