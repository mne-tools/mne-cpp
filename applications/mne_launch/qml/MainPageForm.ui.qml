import QtQuick 2.7
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.0

Item {
    property alias button_close: button_close
    property alias button_mne_scan: button_mne_scan
    property alias animation_mne_scan: animation_mne_scan
    property alias button_mne_browse: button_mne_browse
    property alias button_mne_analyze: button_mne_analyze
    property alias logo: logo

    width: 1040
    height: 650

    Image {
        id: image_background
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: "../resources/tmp_background.jpg"
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
        anchors.rightMargin: 4
    }

    // MNE Scan
    ApplicationButton {
        id: button_mne_scan
        x: 697
        y: 36
        imgSrcNormal: "../resources/icon_mne_scan_white.png"
        imgSrcHover:  "../resources/icon_mne_scan.png"
        imgHeight: 100
        imgWidth: 100
    }

    ScanAnimation {
        id: animation_mne_scan
        anchors.rightMargin: 0
        anchors.bottomMargin: 0
        anchors.leftMargin: 0
        anchors.topMargin: 0
        x_center: button_mne_scan.x
        y_center: button_mne_scan.y+button_mne_scan.height*0.6
        anchors.fill: parent
    }


    // MNE Browse
    ApplicationButton{
        id: button_mne_browse
        x: 867
        y: 216
        imgSrcNormal: "../resources/icon_mne_browse_white.png"
        imgSrcHover:  "../resources/icon_mne_browse.png"
        imgHeight: 100
        imgWidth: 100
    }


    // MNE Analyze
    ApplicationButton{
        id: button_mne_analyze
        x: 749
        y: 428
        imgSrcNormal: "../resources/icon_mne_analyze_white.png"
        imgSrcHover:  "../resources/icon_mne_analyze.png"
        imgHeight: 100
        imgWidth: 100
    }


    Logo {
        id: logo
        x: 25
        y: 20
    }
}
