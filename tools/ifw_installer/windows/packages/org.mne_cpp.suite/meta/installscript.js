var ComponentSelectionPage = null;


//*************************************************************************************************************

var Dir = new function () {
    this.toNativeSparator = function (path) {
        if (installer.value("os") == "win")
            return path.replace(/\//g, '\\');
        return path;
    }
};


//*************************************************************************************************************

function Component() {
    if (installer.isInstaller()) {
        component.loaded.connect(this, Component.prototype.installerLoaded);
        ComponentSelectionPage = gui.pageById(QInstaller.ComponentSelection);

        installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
//        installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
        if (installer.value("os") == "win")
            installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
        installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
    }
}


//*************************************************************************************************************

Component.prototype.installerLoaded = function () {
    if (installer.addWizardPage(component, "TargetWidget", QInstaller.TargetDirectory)) {
        var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
        if (widget != null) {
            widget.targetChooser.clicked.connect(this, Component.prototype.chooseTarget);
            widget.targetDirectory.textChanged.connect(this, Component.prototype.targetChanged);

            widget.windowTitle = "Installation Folder";
            widget.targetDirectory.text = Dir.toNativeSparator(installer.value("TargetDir"));
        }
    }

    if (installer.addWizardPage(component, "InstallationWidget", QInstaller.ComponentSelection)) {
        var widget = gui.pageWidgetByObjectName("DynamicInstallationWidget");
        if (widget != null) {
            widget.customInstall.toggled.connect(this, Component.prototype.customInstallToggled);
            widget.defaultInstall.toggled.connect(this, Component.prototype.defaultInstallToggled);
            widget.completeInstall.toggled.connect(this, Component.prototype.completeInstallToggled);

            widget.defaultInstall.checked = true;
            widget.windowTitle = "Please select a installation type";
        }

//        if (installer.addWizardPage(component, "LicenseWidget", QInstaller.LicenseCheck)) {
//            var widget = gui.pageWidgetByObjectName("DynamicLicenseWidget");
//            if (widget != null) {
//                widget.acceptLicenseRB.toggled.connect(this, Component.prototype.checkAccepted);

//                widget.next.button.enabled = false;
//                widget.complete = false;
//                widget.declineLicenseRB.checked = true;
//                widget.windowTitle = "BSD 3-Clause License";
//            }
//        }

        if (installer.addWizardPage(component, "ReadyToInstallWidget", QInstaller.ReadyForInstallation)) {
            var widget = gui.pageWidgetByObjectName("DynamicReadyToInstallWidget");
            if (widget != null) {
                widget.showDetails.checked = false;
                widget.windowTitle = "Ready to Install";
            }
            var page = gui.pageByObjectName("DynamicReadyToInstallWidget");
            if (page != null) {
                page.entered.connect(this, Component.prototype.readyToInstallWidgetEntered);
            }
        }
    }
}


//*************************************************************************************************************

Component.prototype.targetChanged = function (text) {
    var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
    if (widget != null) {
        if (text != "") {
            if (!installer.fileExists(text + "/components.xml")) {
                widget.complete = true;
                installer.setValue("TargetDir", text);
                return;
            }
        }
        widget.complete = false;
    }
}


//*************************************************************************************************************

Component.prototype.chooseTarget = function () {
    var widget = gui.pageWidgetByObjectName("DynamicTargetWidget");
    if (widget != null) {
        var newTarget = QFileDialog.getExistingDirectory("Choose your target directory.", widget
            .targetDirectory.text);
        if (newTarget != "")
            widget.targetDirectory.text = Dir.toNativeSparator(newTarget);
    }
}


//*************************************************************************************************************

Component.prototype.customInstallToggled = function (checked) {
    if (checked) {
        if (ComponentSelectionPage != null)
            ComponentSelectionPage.selectDefault();
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, true);
    }
}


//*************************************************************************************************************

Component.prototype.defaultInstallToggled = function (checked) {
    if (checked) {
        if (ComponentSelectionPage != null)
            ComponentSelectionPage.selectDefault();
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    }
}


//*************************************************************************************************************

Component.prototype.completeInstallToggled = function (checked) {
    if (checked) {
        if (ComponentSelectionPage != null)
            ComponentSelectionPage.selectAll();
        installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    }
}


//*************************************************************************************************************

//Component.prototype.checkAccepted = function (checked) {
//    var widget = gui.pageWidgetByObjectName("DynamicLicenseWidget");
//    if (widget != null)
//        widget.complete = checked;

////    var result = QMessageBox["question"]("test.quit", "Installer", "Test " + checked, QMessageBox.Ok | QMessageBox.Cancel);

//}


//*************************************************************************************************************

Component.prototype.readyToInstallWidgetEntered = function () {
    var widget = gui.pageWidgetByObjectName("DynamicReadyToInstallWidget");
    if (widget != null) {
        var html = "<b>Components to install:</b><ul>";
        var components = installer.components();
        for (i = 0; i < components.length; ++i) {
            if (components[i].installationRequested())
                html = html + "<li>" + components[i].displayName + "</li>"
        }
        html = html + "</ul>";
        widget.showDetailsBrowser.html = html;
    }
}
