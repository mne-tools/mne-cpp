import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    property alias button_close: button_close

    width: 960
    height: 600

    Image {
        id: image5
        x: 0
        y: -100
        width: 960
        height: 800
        fillMode: Image.PreserveAspectFit
        source: "../resources/Electric_Art_Brain_sdw.jpg"
    }


    // Close
    MenuButton{
        id: button_close

        imgSrc: "../resources/icon_close.svg"
        imgHeight: 16
        imgWidth: 16

        anchors.top: parent.top
        anchors.topMargin: 2
        anchors.right: parent.right
        anchors.rightMargin: 2
    }

    Image {
        id: image1
        x: 692
        y: 56
        width: 100
        height: 100
        source: "../resources/icon_mne_scan_256x256.png"
    }

    Image {
        id: image2
        x: 759
        y: 238
        width: 100
        height: 100
        source: "../resources/icon_browse_256x256.png"
    }

    Image {
        id: image3
        x: 661
        y: 416
        width: 100
        height: 100
        source: "../resources/icon_mne-analyze_256x256.png"
    }

    Image {
        id: image4
        x: 21
        y: 0
        width: 166
        height: 98
        fillMode: Image.PreserveAspectFit
        source: "../resources/MNE-CPP_Logo.svg"
    }
}
