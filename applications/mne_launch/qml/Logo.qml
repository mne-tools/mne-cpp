import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    state: "DEFAULT"

    width: 175
    height: 95

    Image {
        id: logo
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "../resources/MNE-CPP_Logo_color.png"
    }

    states: [
        State {
            name: "DEFAULT"
            PropertyChanges { target: logo; source: "../resources/MNE-CPP_Logo_color.png" }
        },
        State {
            name: "SCAN"
            PropertyChanges { target: logo; source: "../resources/MNE-CPP_Logo_blue.png" }
        },
        State {
            name: "BROWSE"
            PropertyChanges { target: logo; source: "../resources/MNE-CPP_Logo_green.png" }
        },
        State {
            name: "ANALYZE"
            PropertyChanges { target: logo; source: "../resources/MNE-CPP_Logo_red.png" }
        }
    ]

//    transitions: [
//        Transition {
//            from: ""; to: "ACTIVE"
//            ColorAnimation {
//                duration: 200
//            }
//        }
//    ]
}
