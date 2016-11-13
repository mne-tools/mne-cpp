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
