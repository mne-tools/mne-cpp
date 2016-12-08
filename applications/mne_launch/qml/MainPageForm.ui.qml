//=============================================================================================================
/**
* @file     MainPageForm.ui.qml
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
* @brief    Implements the QML MainPage UI component.
*
*/

//*************************************************************************************************************
//=============================================================================================================
// IMPORTS
//=============================================================================================================

import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    property alias button_close: button_close
    property alias button_mne_scan: button_mne_scan
    property alias button_mne_browse: button_mne_browse
    property alias button_mne_analyze: button_mne_analyze

    state: "DEFAULT"

    width: 1040
    height: 650

    Image {
        id: image_background
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "../resources/tmp_background.jpg"
    }

    // Close
    MenuButton {
        id: button_close

        imgSrc: "../resources/icon_close.svg"
        imgHeight: 16
        imgWidth: 16

        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.right: parent.right
        anchors.rightMargin: 4
    }

    // MNE Scan
    ApplicationButton {
        id: button_mne_scan
        x: 728
        y: 34
        imgSrcNormal: "../resources/icon_mne_scan_white.png"
        imgSrcHover:  "../resources/icon_mne_scan.png"
        imgHeight: 100
        imgWidth: 100
    }
    Text {
        id: text_mne_scan
        color: "#ffffff"
        anchors.right: button_mne_scan.left
        anchors.rightMargin: 10
        anchors.top: button_mne_scan.top
        anchors.topMargin: button_mne_scan.height/6
        text: qsTr("MNE Scan")
        font.bold: true
        font.family: "Arial"
        opacity: 0.8
        font.pixelSize: 22
    }
    CenterDot {
        id: center_dot_scan
        x: 553
        y: 207
    }
    ConnectorLine {
        id: connector_line_scan
        x_start: button_mne_scan.x
        y_start: button_mne_scan.y+button_mne_scan.height*0.5
        x_end: center_dot_scan.x_center
        y_end: center_dot_scan.y_center
        anchors.fill: parent
    }


    // MNE Browse
    ApplicationButton{
        id: button_mne_browse
        x: 867
        y: 216
        imgSrcNormal: "../resources/icon_mne_browse_white.png"
        imgSrcHover:  "../resources/icon_mne_browse.png"
        imgHeight: 100
        imgWidth: 100
    }
    Text {
        id: text_mne_browse
        color: "#ffffff"
        anchors.right: button_mne_browse.left
        anchors.rightMargin: 10
        anchors.top: button_mne_browse.top
        anchors.topMargin: button_mne_browse.height/6
        text: qsTr("MNE Browse")
        font.bold: true
        font.family: "Arial"
        opacity: 0.8
        font.pixelSize: 22
    }
    CenterDot{
        id: center_dot_browse
        x: 604
        y: 307
    }
    ConnectorLine {
        id: connector_line_browse
        x_start: button_mne_browse.x
        y_start: button_mne_browse.y+button_mne_browse.height*0.5
        x_end: center_dot_browse.x_center
        y_end: center_dot_browse.y_center
        anchors.fill: parent
    }

    // MNE Analyze
    ApplicationButton{
        id: button_mne_analyze
        x: 844
        y: 437
        imgSrcNormal: "../resources/icon_mne_analyze_white.png"
        imgSrcHover:  "../resources/icon_mne_analyze.png"
        imgHeight: 100
        imgWidth: 100
    }
    Text {
        id: text_mne_analyze
        color: "#ffffff"
        anchors.right: button_mne_analyze.left
        anchors.rightMargin: 10
        anchors.top: button_mne_analyze.top
        anchors.topMargin: button_mne_analyze.height/6
        text: qsTr("MNE Analyze")
        font.bold: true
        font.family: "Arial"
        opacity: 0.8
        font.pixelSize: 22
    }
    CenterDot{
        id: center_dot_analyze
        x: 584
        y: 437
    }
    ConnectorLine {
        id: connector_line_analyze
        x_start: button_mne_analyze.x
        y_start: button_mne_analyze.y+button_mne_analyze.height*0.5
        x_end: center_dot_analyze.x_center
        y_end: center_dot_analyze.y_center
        anchors.fill: parent
    }

    // MNE-CPP
    Logo {
        id: logo
        x: 25
        y: 20
    }

    //Sample Data
    SampleDataStatus {
        id: sampleDataStatusLine
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
    }

    states: [
        State {
            name: "DEFAULT"
            PropertyChanges { target: logo; state: "DEFAULT" }
            PropertyChanges { target: center_dot_scan; visible: false }
            PropertyChanges { target: center_dot_browse; visible: false }
            PropertyChanges { target: center_dot_analyze; visible: false }
            PropertyChanges { target: connector_line_scan; visible: false }
            PropertyChanges { target: connector_line_browse; visible: false }
            PropertyChanges { target: connector_line_analyze; visible: false }
            PropertyChanges { target: text_mne_scan; visible: false }
            PropertyChanges { target: text_mne_browse; visible: false }
            PropertyChanges { target: text_mne_analyze; visible: false }
        },
        State {
            name: "SCAN"
            PropertyChanges { target: logo; state: "SCAN" }
            PropertyChanges { target: center_dot_scan; visible: true }
            PropertyChanges { target: center_dot_browse; visible: false }
            PropertyChanges { target: center_dot_analyze; visible: false }
            PropertyChanges { target: connector_line_scan; visible: true }
            PropertyChanges { target: connector_line_browse; visible: false }
            PropertyChanges { target: connector_line_analyze; visible: false }
            PropertyChanges { target: text_mne_scan; visible: true }
            PropertyChanges { target: text_mne_browse; visible: false }
            PropertyChanges { target: text_mne_analyze; visible: false }
        },
        State {
            name: "BROWSE"
            PropertyChanges { target: logo; state: "BROWSE" }
            PropertyChanges { target: center_dot_scan; visible: false }
            PropertyChanges { target: center_dot_browse; visible: true }
            PropertyChanges { target: center_dot_analyze; visible: false }
            PropertyChanges { target: connector_line_scan; visible: false }
            PropertyChanges { target: connector_line_browse; visible: true }
            PropertyChanges { target: connector_line_analyze; visible: false }
            PropertyChanges { target: text_mne_scan; visible: false }
            PropertyChanges { target: text_mne_browse; visible: true }
            PropertyChanges { target: text_mne_analyze; visible: false }
        },
        State {
            name: "ANALYZE"
            PropertyChanges { target: logo; state: "ANALYZE" }
            PropertyChanges { target: center_dot_scan; visible: false }
            PropertyChanges { target: center_dot_browse; visible: false }
            PropertyChanges { target: center_dot_analyze; visible: true }
            PropertyChanges { target: connector_line_scan; visible: false }
            PropertyChanges { target: connector_line_browse; visible: false }
            PropertyChanges { target: connector_line_analyze; visible: true }
            PropertyChanges { target: text_mne_scan; visible: false }
            PropertyChanges { target: text_mne_browse; visible: false }
            PropertyChanges { target: text_mne_analyze; visible: true }
        }
    ]
}
