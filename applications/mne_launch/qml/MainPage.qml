import QtQuick 2.7

MainPageForm {
    button_close.onClicked: {
        Qt.quit();
    }

    // MNE Scan
    button_mne_scan.onClicked: {
        console.log("MNE Scan clicked!");
    }

    button_mne_scan.onHoveredChanged: {
        if ( button_mne_scan.hovered ) {
            animation_mne_scan.active = true
            logo.state = "SCAN"
        }
        else {
            animation_mne_scan.active = false
            logo.state = "DEFAULT"
        }
    }

    // MNE Browse
    button_mne_browse.onClicked: {
        console.log("MNE Browse clicked!");
    }

    button_mne_browse.onHoveredChanged: {
        if ( button_mne_browse.hovered ) {
//            animation_mne_browse.active = true
            logo.state = "BROWSE"
        }
        else {
//            animation_mne_browse.active = false
            logo.state = "DEFAULT"
        }
    }

    // MNE Analyze
    button_mne_analyze.onClicked: {
        console.log("MNE Analyze clicked!");
    }

    button_mne_analyze.onHoveredChanged: {
        if ( button_mne_analyze.hovered ) {
//            animation_mne_analyze.active = true
            logo.state = "ANALYZE"
        }
        else {
//            animation_mne_analyze.active = false
            logo.state = "DEFAULT"
        }
    }
}
