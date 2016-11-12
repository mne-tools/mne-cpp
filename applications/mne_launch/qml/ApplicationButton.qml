import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property alias imgSrcNormal: imageNormalItem.source
    property alias imgSrcHover: imageHoverItem.source
    property alias imgWidth: imageNormalItem.sourceSize.width
    property alias imgHeight: imageNormalItem.sourceSize.height

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
        anchors.centerIn: parent
        visible: true
    }

    Image {
        id: imageHoverItem
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
            name: "stateClicked"
            when: mouseArea.pressed === true

            PropertyChanges {
                target: buttonRect
                border.color: "black"
            }
            PropertyChanges {
                target: imageNormalItem
                visible: true
            }
            PropertyChanges {
                target: imageHoverItem
                visible: false
            }
        },
        State {
            name: "stateHover"
            when: mouseArea.containsMouse === true

            PropertyChanges {
                target: buttonRect
                border.color: "white"
            }
            PropertyChanges {
                target: imageNormalItem
                visible: false
            }
            PropertyChanges {
                target: imageHoverItem
                visible: true
            }
        }
    ]

    transitions: [
        Transition {
            from: ""; to: "stateHover"
            ColorAnimation {
                duration: 300
            }
        }
    ]
}
