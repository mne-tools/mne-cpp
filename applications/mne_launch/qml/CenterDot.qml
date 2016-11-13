//=============================================================================================================
/**
* @file     CenterDot.qml
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh. All rights reserved.
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
* @brief    Implements QML CenterDot component.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// IMPORTS
//=============================================================================================================

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property int center_radius: 5

    property int x_center: x + center_radius
    property int y_center: y + center_radius

    width: center_radius*2
    height: center_radius*2

    Rectangle {
        anchors.fill: parent
        color: "white"
        radius: center_radius
    }

    Rectangle {
        id: echo_circle

        property int val: 0
        property int max_radius: 30

        anchors.centerIn: parent

        width: val*2
        height: val*2

        radius: val

        color: "transparent"

        opacity: (max_radius - val)/max_radius

        border.color: "white"
        border.width: 2

        NumberAnimation on val {
            from: 0
            to: echo_circle.max_radius
            loops: Animation.Infinite
            duration: 2000
            easing.type: Easing.OutQuad
        }
    }
}
