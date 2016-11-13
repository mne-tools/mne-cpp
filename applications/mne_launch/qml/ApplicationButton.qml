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
            name: "DEFAULT"
            PropertyChanges { target: buttonRect; border.color: "transparent" }
            PropertyChanges { target: container; hovered: false }
            PropertyChanges { target: imageNormalItem; visible: true }
            PropertyChanges { target: imageHoverItem; visible: false }
        },
        State {
            name: "HOVERED"
            when: mouseArea.containsMouse === true && mouseArea.pressed !== true

            PropertyChanges { target: buttonRect; border.color: "white" }
            PropertyChanges { target: container; hovered: true }
            PropertyChanges { target: imageNormalItem; visible: false }
            PropertyChanges { target: imageHoverItem; visible: true }
        },
        State {
            name: "PRESSED"
            when: mouseArea.pressed === true

            PropertyChanges { target: buttonRect; border.color: "black" }
            PropertyChanges { target: container; hovered: true }
            PropertyChanges { target: imageNormalItem; visible: true }
            PropertyChanges { target: imageHoverItem; visible: false }
        }
    ]

//    transitions: [
//        Transition {
//            from: ""; to: "stateHover"
//            ColorAnimation {
//                duration: 100
//            }
//        }
//    ]
}
