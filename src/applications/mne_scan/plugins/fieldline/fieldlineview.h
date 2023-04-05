//=============================================================================================================
/**
 * @file     fl_rack.h
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
 * @brief     FieldlineView class declaration.
 *
 */

#ifndef FIELDLINE_FIELDLINEVIEW_H
#define FIELDLINE_FIELDLINEVIEW_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

// #include "disp/viewers/led_indicator.h"

// #include <memory>
// #include <vector>
//
// //=============================================================================================================
// // QT INCLUDES
// //=============================================================================================================
//
// #include <QColor>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

class QWidget;
 namespace Ui {
    class fieldline_view_ui
//     class fieldline_rack;
//     class fieldline_chassis;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace FIELDLINEPLUGIN {

class Fieldline;
// class fieldline_chassis;

//=============================================================================================================
class FieldlineView /*: public QWidget */ {
    // Q_OBJECT

public:
    FieldlineView(Fieldline* parent);

    QWidget* getWidget() const;
    // FieldlineView(int num_chassis, int sensors_per_chassis, QWidget *parent = nullptr);
    // FieldlineView(int num_chassis, QWidget *parent = nullptr);
    // ~FieldlineView();
    //
    // void configure(int num_chassis);
    // void configure(int num_chassis, int num_sensors);
    //
    // void clear();
    //
    // void setColor(size_t chassis_id, size_t sensor_num, const QColor& color);
    // void setColor(size_t chassis_id, size_t sensor_num, const QColor& color, bool blinking);
    //
    // void setChassisColor(size_t chassis_id, const QColor& color);
    // void setChassisColor(size_t chassis_id, const QColor& color, bool blinking);
    //
    // void setAllColor(const QColor& color);
    // void setAllColor(const QColor &color, bool blinking);
    //
    // void setBlinkState(size_t chassis_id, size_t sensor_num, bool blinking);
    // void setChassisBlinkState(size_t chassis_id, bool blinking);
    // void setAllBlinkState(bool blinking);
    //
    // static void setDefaultNumSensors(int num_sensors);
private:
    // static int default_num_sensors;
    //
    // Ui::fieldline_rack* ui;
    // std::vector<fieldline_chassis*> chassis;
    Fieldline* m_pFieldlinePlugin;
};

//=============================================================================================================

// class fieldline_chassis : public QWidget
// {
//     Q_OBJECT
//
// public:
//     fl_chassis(int num_sensors, QWidget *parent = nullptr);
//     ~fl_chassis();
//
//     void setColor(size_t sensor_num, const QColor& color);
//     void setColor(size_t sensor_num, const QColor& color, bool blinking);
//     void setColor(const QColor& color);
//     void setColor(const QColor& color, bool blinking);
//
//     void setBlinkState(size_t sensor_num, bool blinking);
//     void setBlinkState(bool blinking);
//
// signals:
//     void clicked(int sensor, const QPoint& pos);
//
// private slots:
//     void rightClickMenu(int sensor, const QPoint& pos);
//
// private:
//     Ui::fl_chassis* ui;
//     std::vector<DISPLIB::LEDIndicator*> sensors;
// };


}  // namespace FIELDLINEPLUGIN

#endif  // FIELDLINE_UI_VIEW_H
