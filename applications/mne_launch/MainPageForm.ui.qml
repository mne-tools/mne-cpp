import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    property alias button1: button1
    property alias button2: button2


    Image {
        id: image5
        x: 0
        y: -42
        width: 960
        height: 800
        fillMode: Image.PreserveAspectFit
        source: "resources/Electric_Art_Brain_sdw.jpg"
    }

    RowLayout {
        anchors.verticalCenterOffset: -39
        anchors.horizontalCenterOffset: -157
        anchors.centerIn: parent

        Button {
            id: button1
            text: qsTr("Press Me 1")
        }

        Button {
            id: button2
            text: qsTr("Press Me 2")
        }
    }

    Image {
        id: image1
        x: 430
        y: -10
        width: 100
        height: 100
        source: "resources/icon_mne_scan_256x256.png"
    }

    Image {
        id: image2
        x: 685
        y: 67
        width: 100
        height: 100
        source: "resources/icon_browse_256x256.png"
    }

    Image {
        id: image3
        x: 797
        y: 330
        width: 100
        height: 100
        source: "resources/icon_mne-analyze_256x256.png"
    }

    Image {
        id: image4
        x: 37
        y: 507
        width: 166
        height: 98
        fillMode: Image.PreserveAspectFit
        source: "resources/MNE-CPP_Logo.svg"
    }

}
