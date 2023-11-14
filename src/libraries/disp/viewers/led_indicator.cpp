//=============================================================================================================
/**
 * @file     led_indicator.cpp
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
 * @brief     LEDIndicator class definition.
 *
 */

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "led_indicator.h"
#include "ui_led_indicator.h"

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QScrollBar>
#include <QMouseEvent>
#include <QDebug>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// USED NAMESPACES
//=============================================================================================================

using DISPLIB::LEDIndicator;

//=============================================================================================================
// DEFINE STATIC METHODS
//=============================================================================================================

QColor LEDIndicator::default_color = Qt::red;
QString LEDIndicator::default_label = "XX";

//=============================================================================================================
// DEFINE MEMBER METHODS
//=============================================================================================================

LEDIndicator::LEDIndicator(const QString& label, const QColor& led_color, QWidget *parent)
: QWidget(parent)
, ui(std::make_unique<Ui::led_indicator>())
, off_brush(QBrush(Qt::transparent))
, on_brush(QBrush(led_color))
, blink_time_ms(200)
, blink_state(false)
{
    m_pScene = std::make_unique<QGraphicsScene>();

    ui->setupUi(this);
    ui->graphicsView->setStyleSheet("background:transparent");
    ui->graphicsView->horizontalScrollBar()->hide();
    ui->graphicsView->verticalScrollBar()->hide();
    ui->label->setText(label);
    ui->graphicsView->setScene(m_pScene.get());

    circle_led = m_pScene->addEllipse(0,0,this->width()/3,this->width()/3, QPen(Qt::black), on_brush);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
}

//=============================================================================================================

LEDIndicator::LEDIndicator(const QString& label, QWidget *parent)
: LEDIndicator(label, default_color, parent)
{
}

//=============================================================================================================

LEDIndicator::LEDIndicator(QWidget *parent)
: LEDIndicator(default_label, default_color, parent)
{
}

//=============================================================================================================

LEDIndicator::~LEDIndicator()
{
}

//=============================================================================================================

void LEDIndicator::setLabel(const QString& label)
{
    ui->label->setText(label);
}

//=============================================================================================================

void LEDIndicator::setBlink(bool state)
{
    state ? turnOnBlink() : turnOffBlink();
}

//=============================================================================================================

void LEDIndicator::setColor(const QColor& color)
{
    on_brush.setColor(color);
}

//=============================================================================================================

void LEDIndicator::resizeEvent(QResizeEvent *event)
{
    auto bounds = m_pScene->itemsBoundingRect();
    bounds.setWidth(bounds.width() * 1.2);
    bounds.setHeight(bounds.height() * 1.2);
    ui->graphicsView->fitInView(bounds, Qt::KeepAspectRatio);

    QWidget::resizeEvent(event);
}

//=============================================================================================================

void LEDIndicator::turnOnBlink()
{
    connect(&blink_timer, &QTimer::timeout,
            this, &LEDIndicator::handleBlink, Qt::UniqueConnection);
    blink_timer.start(blink_time_ms);
}

//=============================================================================================================

void LEDIndicator::turnOffBlink()
{
    blink_timer.stop();
    circle_led->setBrush(on_brush);
}

//=============================================================================================================

void LEDIndicator::handleBlink()
{
    blink_state ? circle_led->setBrush(off_brush)
                : circle_led->setBrush(on_brush);

    blink_state = !blink_state;
}
