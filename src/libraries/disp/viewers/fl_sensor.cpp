//=============================================================================================================
/**
 * @file     fl_sensor.cpp
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
 * @brief     FieldlineSensor class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================


#include "fl_sensor.h"
#include "ui_fl_sensor.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using DISPLIB::FieldlineSensor;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

QColor FieldlineSensor::default_color = Qt::red;
QString FieldlineSensor::default_label = "XX";

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

FieldlineSensor::FieldlineSensor(const QString& label, const QColor& led_color, QWidget *parent)
: QWidget(parent)
, ui(std::make_unique<Ui::fl_sensor>())
, blink_brush(QBrush(Qt::transparent))
, on_brush(QBrush(led_color))
, blink_time_ms(200)
{
    m_pScene = std::make_unique<QGraphicsScene>();

    ui->setupUi(this);
    ui->graphicsView->setStyleSheet("background:transparent");
    ui->label->setText(label);
    ui->graphicsView->setScene(m_pScene.get());

    circle_led = m_pScene->addEllipse(0,0,this->width()/3,this->width()/3, QPen(Qt::black), on_brush);
}

//=============================================================================================================

FieldlineSensor::FieldlineSensor(const QString& label, QWidget *parent)
: FieldlineSensor(label, default_color, parent)
{
}

//=============================================================================================================

FieldlineSensor::FieldlineSensor(QWidget *parent)
: FieldlineSensor(default_label, default_color, parent)
{
}

//=============================================================================================================

FieldlineSensor::~FieldlineSensor()
{
}

//=============================================================================================================

void FieldlineSensor::setLabel(const QString& label)
{
    ui->label->setText(label);
}

//=============================================================================================================

void FieldlineSensor::resizeEvent(QResizeEvent *event)
{
    auto bounds = m_pScene->itemsBoundingRect();
    bounds.setWidth(bounds.width() * 1.2);
    bounds.setHeight(bounds.height() * 1.2);
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);

    QWidget::resizeEvent(event);
}

//=============================================================================================================

void FieldlineSensor::setBlink(bool state)
{
    state ? turnOnBlink() : turnOffBlink();
}

//=============================================================================================================

void FieldlineSensor::turnOnBlink()
{
    connect(&blink_timer, &QTimer::timeout,
            this, &FieldlineSensor::handleBlink, Qt::UniqueConnection);
    blink_timer.start(blink_time_ms);
}

//=============================================================================================================

void FieldlineSensor::turnOffBlink()
{
    blink_timer.stop();
    circle_led->setBrush(on_brush);
}

//=============================================================================================================

void FieldlineSensor::handleBlink()
{
    static bool blink_state = false;

    blink_state ? circle_led->setBrush(blink_brush)
                : circle_led->setBrush(on_brush);

    blink_state = !blink_state;
}
