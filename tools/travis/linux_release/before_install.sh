#!/bin/bash


#///no longer using repositories for Qt5.10.1

# Update Repositories
#sudo add-apt-repository ppa:beineri/opt-qt-5.10.1-trusty -y
#sudo apt-get update -qq

#///


#Get Linuxdeployqt and change permissions, Same with qt5 offline installer

wget https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage
chmod a+x linuxdeployqt-continuous-x86_64.AppImage 

wget http://download.qt.io/archive/qt/5.10/5.10.1/qt-opensource-linux-x64-5.10.1.run
chmod a+x qt-opensource-linux-x64-5.10.1.run

#lazy resolve of differences in standard directories. 
mkdir -p /opt/Qt-5.10.1
ln -sfvn /opt/Qt-5.10.1  /opt/qt510
ln -sfvn /opt/Qt-5.10.1  /opt/qt5

#create non interactive scripts, installs to Qt-5.10.1

cat > qt5-noninteractive.qs << "EOF"

// Emacs mode hint: -*- mode: JavaScript -*-

function Controller() {
    installer.autoRejectMessageBoxes();
	installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("stopProcessesForUpdates", QMessageBox.Ignore);
    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    })
}

Controller.prototype.WelcomePageCallback = function() {
    // click delay here because the next button is initially disabled for ~1 second
    gui.clickButton(buttons.NextButton, 3000);
}

Controller.prototype.CredentialsPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    gui.currentPageWidget().TargetDirectoryLineEdit.setText(installer.value("/opt/Qt-5.10.1");
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    var widget = gui.currentPageWidget();

    widget.deselectAll();
    widget.selectComponent("qt.qt5.5101.gcc_64");
    widget.selectComponent("qt.qt5.5101.qtcharts");

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.StartMenuDirectoryPageCallback = function() {
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function() {
var checkBoxForm = gui.currentPageWidget().LaunchQtCreatorCheckBoxForm;
if (checkBoxForm && checkBoxForm.launchQtCreatorCheckBox) {
    checkBoxForm.launchQtCreatorCheckBox.checked = false;
}
    gui.clickButton(buttons.FinishButton);
}

EOF
