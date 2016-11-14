//=============================================================================================================
/**
* @file     ApplicationButton.qml
* @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
*           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
* @version  1.0
* @date     November, 2016
*
* @section  LICENSE
*
* Copyright (C) 2016, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
* @brief    Implements QML ApplicationButton component.
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

    property string imgSrcNormal: "../resources/icon_mne_scan_white.png"
    property string imgSrcHover: "../resources/icon_mne_scan.png"
    property int imgWidth: 100
    property int imgHeight: 100

    property bool hovered: false

    signal clicked

    width: imgWidth + 20
    height: imgHeight + 20

    Rectangle{
        id: buttonRect
        color: "transparent"
        anchors.fill: container
        border {
            width: 2
            color: "transparent"
        }
        radius: 4
        opacity: 0.6
    }

    Image {
        id: imageNormalItem
        source: imgSrcNormal
        anchors.centerIn: parent
        sourceSize.width: imgWidth
        sourceSize.height: imgHeight
        visible: true
    }

    Image {
        id: imageHoverItem
        source: imgSrcHover
        anchors.centerIn: parent
        sourceSize.width: imgWidth
        sourceSize.height: imgHeight
        visible: false
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
            name: "HOVERED"
            when: mouseArea.containsMouse === true && mouseArea.pressed !== true

            PropertyChanges { target: buttonRect; border.color: "white"; opacity: 0.6 }
            PropertyChanges { target: container; hovered: true }
            PropertyChanges { target: imageNormalItem; visible: false }
            PropertyChanges { target: imageHoverItem; visible: true }
        },
        State {
            name: "PRESSED"
            when: mouseArea.pressed === true

            PropertyChanges { target: buttonRect; border.color: "black"; opacity: 0.2 }
            PropertyChanges { target: container; hovered: true }
            PropertyChanges { target: imageNormalItem; visible: true }
            PropertyChanges { target: imageHoverItem; visible: false }
        }
    ]
}
