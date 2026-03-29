// controlscript.qs - MNE-CPP Installer control script
//
// This script customizes the installer wizard behaviour and enables
// headless (non-interactive) bootstrap installs:
//
//   ./MNE-CPP-Installer --script controlscript.qs
//
// Or more commonly via the bootstrap shell script:
//
//   curl -fsSL https://mne-cpp.github.io/install.sh | sh
//
// The installer supports these environment-variable overrides:
//
//   MNE_CPP_INSTALL_DIR   - Override the target directory
//                            (default: ~/MNE-CPP)
//   MNE_CPP_VARIANT       - Select "dynamic" or "static"
//                            (default: dynamic)
//   MNE_CPP_CLI_ONLY      - Set to "1" to skip desktop integration and
//                            optional add-ons during headless installs

function Controller()
{
    // Auto-accept the welcome page in non-interactive mode
    installer.autoRejectMessageBoxes();
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory",
                                           QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("stopProcessesForUpdates",
                                           QMessageBox.Ignore);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    var targetDir = installer.environmentVariable("MNE_CPP_INSTALL_DIR");
    if (targetDir !== "") {
        gui.currentPageWidget().TargetDirectoryLineEdit.setText(targetDir);
    }
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.BinaryVariantPageCallback = function()
{
    var page = gui.currentPageWidget();
    var variant = installer.environmentVariable("MNE_CPP_VARIANT");

    if (page && variant === "static") {
        var radioStatic = page.findChild("radioStatic");
        if (radioStatic) {
            radioStatic.checked = true;
        }
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var page = gui.currentPageWidget();
    var cliOnly = installer.environmentVariable("MNE_CPP_CLI_ONLY");

    if (cliOnly === "1") {
        // Deselect everything first
        page.deselectAll();

        // Select only the core payload and PATH configuration.
        page.selectComponent("org.mnecpp.core");
        page.selectComponent("org.mnecpp.pathconfig");
    }

    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    gui.currentPageWidget().AcceptLicenseRadioButton.setChecked(true);
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.PerformInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.FinishedPageCallback = function()
{
    // Uncheck "Run MNE-CPP" on finish
    var checkBox = gui.currentPageWidget().RunItCheckBox;
    if (checkBox) {
        checkBox.checked = false;
    }
    gui.clickButton(buttons.FinishButton);
}
