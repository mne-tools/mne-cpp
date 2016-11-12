import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property alias imgSrc: imageItem.source
    property alias imgWidth: imageItem.sourceSize.width //Source Size rerenders SVG
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
            name: "stateClicked"
            when: mouseArea.pressed === true

            PropertyChanges {
                target: imageColorOverlay
                color: "black"
            }
        },
        State {
            name: "stateHover"
            when: mouseArea.containsMouse === true

            PropertyChanges {
                target: imageColorOverlay
                color: "white"
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
