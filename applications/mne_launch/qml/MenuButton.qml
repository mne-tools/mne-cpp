//=============================================================================================================
/**
* @file     MenuButton.qml
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
* @brief    Implements QML MenuButton component.
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

    property alias imgSrc: imageItem.source
    property alias imgWidth: imageItem.sourceSize.width
    property alias imgHeight: imageItem.sourceSize.height

    signal clicked

    width: 40
    height: 40

    Image {
        id: imageItem

        anchors.centerIn: parent
    }

    ColorOverlay {
        id: imageColorOverlay
        anchors.fill: imageItem
        source: imageItem
        color: "#3e5c59"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            parent.clicked()
        }
    }

    states: [
        State {
            name: "PRESSED"
            when: mouseArea.pressed === true

            PropertyChanges {
                target: imageColorOverlay
                color: "black"
            }
        },
        State {
            name: "HOVERED"
            when: mouseArea.containsMouse === true

            PropertyChanges {
                target: imageColorOverlay
                color: "white"
            }
        }
    ]

    transitions: [
        Transition {
            from: ""; to: "HOVERED"
            ColorAnimation {
                duration: 300
            }
        }
    ]
}
