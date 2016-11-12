import QtQuick 2.7

MainPageForm {
    button_close.onClicked: {
        Qt.quit();
    }

    button_mne_scan.onClicked: {
        console.log("MNE Scan clicked!");
    }

    button_mne_browse.onClicked: {
        console.log("MNE Browse clicked!");
    }

    button_mne_analyze.onClicked: {
        console.log("MNE Analyze clicked!");
    }

}
