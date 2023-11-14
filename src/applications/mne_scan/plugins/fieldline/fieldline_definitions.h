//=============================================================================================================
/**
 * @file     fieldline_view.h
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

#ifndef FIELDLINE_FIELDLINEDEFINITIONS_H
#define FIELDLINE_FIELDLINEDEFINITIONS_H

namespace FIELDLINEPLUGIN {

enum class FieldlineDataType {
    DATA_BZ = 0,
    DATA_BY = 1,
    DATA_BX = 2
};

enum class FieldLineWaveType {
    WAVE_OFF = 0,
    WAVE_RAMP = 1,
    WAVE_SINE = 2,
};

enum class FieldlineSensorStatusType {
    SENSOR_OFF = 0,
    SENSOR_RESTARTING = 1,
    SENSOR_RESTARTED = 2,
    SENSOR_COARSE_ZEROING = 3,
    SENSOR_COARSE_ZEROED = 4,
    SENSOR_FINE_ZEROING = 5,
    SENSOR_FINE_ZEROED = 6,
    SENSOR_ERROR = 7,
    SENSOR_READY = 8,
};

enum class FieldLineConnectStatusType {
    CONNECT_OK = 0,
    CONNECT_NOT_READY = 1,
    CONNECT_CHASSIS_MISSING = 2,
};

}  // namespace FIELDLINEPLUGIN

#endif  // FIELDLINE_FIELDLINEDEFINITIONS_H
