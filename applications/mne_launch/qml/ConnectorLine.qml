import QtQuick 2.7
import QtQuick.Controls 2.0
import QtGraphicalEffects 1.0

Item{
    id: container

    property int x_start: 0
    property int y_start: 0

    property int x_end: 100
    property int y_end: 100

    property int x_mid: x_start + (x_end - x_start) / 2
    property int y_mid: y_start

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
}
