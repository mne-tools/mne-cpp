import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

ApplicationWindow {
    visible: true
    width: 1040
    height: 650
    minimumWidth: width
    minimumHeight: height
    maximumWidth: width
    maximumHeight: height
    title: qsTr("MNE Launch")

    flags: Qt.FramelessWindowHint | Qt.Window

    MainPage {
        anchors.fill: parent
    }
}
