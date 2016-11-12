import QtQuick 2.7

MainPageForm {
    button_close.onClicked: {
        console.log("Button Close clicked.");
        Qt.quit();
    }
}
