//=============================================================================================================
/**
 * @file     led_indicator.h
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
 * @brief     FieldlineSensor class declaration.
 *
 */

#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include "../disp_global.h"

#include <memory>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>
#include <QGraphicsScene>
#include <QTimer>
#include <QGraphicsEllipseItem>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
    class led_indicator;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace DISPLIB
{

class LEDIndicator : public QWidget
{
    Q_OBJECT

public:
    LEDIndicator(const QString& label, const QColor& led_color, QWidget *parent = nullptr);
    LEDIndicator(const QString& label, QWidget *parent = nullptr);
    explicit LEDIndicator(QWidget *parent = nullptr);
    ~LEDIndicator();

    void setLabel(const QString& label);
    void setBlink(bool state);
    void setColor(const QColor& color);

protected:
    virtual void resizeEvent(QResizeEvent *event);

private:
    void turnOnBlink();
    void turnOffBlink();
    void handleBlink();

    std::unique_ptr<Ui::led_indicator> ui;
    std::unique_ptr<QGraphicsScene> m_pScene;

    int blink_time_ms;
    QTimer blink_timer;
    bool blink_state;

    static QColor default_color;
    static QString default_label;

    QGraphicsEllipseItem* circle_led;
    QBrush off_brush;
    QBrush on_brush;
};
}//namespace DISPLIB

#endif // LED_INDICATOR_H
