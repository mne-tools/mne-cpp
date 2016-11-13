import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property bool active: false

    property int x_center: 0
    property int y_center: 0

    property int x_start: x_center
    property int y_start: y_center

    property int x_mid: x_start-100
    property int y_mid: y_start

    property int x_end: x_mid-50
    property int y_end: y_mid+70

    Component.onCompleted: {
        lines_mne_scan.visible = false;
    }


    Canvas {
        id: lines_mne_scan

        // canvas size
        width: parent.width; height: parent.height

        // handler to override for drawing
        onPaint: {
            // get context to draw with
            var ctx = getContext("2d")
            // setup the stroke
            ctx.lineWidth = 2
            ctx.strokeStyle = "white"
            // begin a new path to draw
            ctx.beginPath()
            // start point
            ctx.moveTo(x_start,y_start)
            // mid line
            ctx.lineTo(x_mid,y_mid)
            // end line
            ctx.lineTo(x_end,y_end)
            // stroke using line width and stroke style
            ctx.stroke()
        }
    }


    states: [
        State {
            name: "ACTIVE"
            when: container.active === true

            PropertyChanges {
                target: lines_mne_scan
                visible: true
            }
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
