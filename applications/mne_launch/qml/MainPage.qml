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
        if ( button_mne_scan.hovered )
            state = "SCAN"
        else
            state = "DEFAULT"
    }

    // MNE Browse
    button_mne_browse.onClicked: {
        console.log("MNE Browse clicked!");
    }

    button_mne_browse.onHoveredChanged: {
        if ( button_mne_browse.hovered )
            state = "BROWSE"
        else
            state = "DEFAULT"
    }

    // MNE Analyze
    button_mne_analyze.onClicked: {
        console.log("MNE Analyze clicked!");
    }

    button_mne_analyze.onHoveredChanged: {
        if ( button_mne_analyze.hovered )
            state = "ANALYZE"
        else
            state = "DEFAULT"
    }

    states: [
        State {
            name: "DEFAULT"
        },
        State {
            name: "SCAN"
        },
        State {
            name: "BROWSE"
        },
        State {
            name: "ANALYZE"
        }
    ]
}
