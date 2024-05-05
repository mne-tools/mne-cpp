//=============================================================================================================
/**
 * @file     fieldline_view_chassis.h
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
 * @brief     FieldlineView class declaration.
 *
 */

#ifndef FIELDLINEPLUGIN_FIELDLINEVIEWCHASSIS_H
#define FIELDLINEPLUGIN_FIELDLINEVIEWCHASSIS_H

//=============================================================================================================
// INCLUDES
//=============================================================================================================

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

//=============================================================================================================
// QT INCLUDES
//=============================================================================================================

#include <QWidget>

//=============================================================================================================
// EIGEN INCLUDES
//=============================================================================================================

//=============================================================================================================
// FORWARD DECLARATIONS
//=============================================================================================================

namespace Ui {
class uiFieldlineViewChassis;
}

//=============================================================================================================
// DEFINE NAMESPACE DISPLIB
//=============================================================================================================

namespace FIELDLINEPLUGIN {

class FieldlineView;
class FieldlineViewSensor;

//=============================================================================================================

class FieldlineViewChassis : public QWidget
{
    Q_OBJECT

 public:
    explicit FieldlineViewChassis(FieldlineView *parent, int num);
    ~FieldlineViewChassis();
    void setActive();

 private:
    void createSensors();
    void updateSensorLeds();
    FieldlineView *m_pFieldlineView;
    Ui::uiFieldlineViewChassis* m_pUi;
    std::vector<FieldlineViewSensor*> m_pSensors;
    int chassisNum;
    int numSensors;
    std::chrono::milliseconds sensorLedUpdateFreq;
    std::thread updateSensorLedThread;
    std::atomic<bool> chassisActive;
    std::atomic<bool> chassisRunning;
};

}  // namespace FIELDLINEPLUGIN

#endif  //  FIELDLINEPLUGIN_FIELDLINEVIEWCHASSIS_H
