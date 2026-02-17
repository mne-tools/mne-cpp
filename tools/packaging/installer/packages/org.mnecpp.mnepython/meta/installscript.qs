// installscript.qs — MNE-Python post-install hook
//
// Adds a custom page that lets the user choose between:
//   - Install for current user only (pip install --user mne)
//   - Install system-wide           (requires sudo / admin)
//
// The selected mode is passed as --user or --global to the
// install_mne_python script.

var mnePythonPage;

function Component()
{
    // Register a custom page shown after the target-directory page.
    installer.addWizardPage(component, "MnePythonModePage", QInstaller.TargetDirectory);

    mnePythonPage = gui.pageWidgetByObjectName("DynamicMnePythonModePage");
    if (mnePythonPage) {
        // Create the UI dynamically (no .ui file needed)
        mnePythonPage.title = "MNE-Python Installation Mode";
        mnePythonPage.description =
            "Choose how MNE-Python should be installed.  " +
            "'User' installs into your local site-packages (no admin rights needed).  " +
            "'Global' installs system-wide (requires sudo / administrator privileges).";

        // Radio buttons
        var layout = new QVBoxLayout(mnePythonPage);

        var radioUser = new QRadioButton("Install for current user only  (pip install --user mne)");
        radioUser.objectName = "radioUser";
        radioUser.checked = true;
        layout.addWidget(radioUser);

        var radioGlobal = new QRadioButton("Install system-wide  (requires sudo / administrator)");
        radioGlobal.objectName = "radioGlobal";
        layout.addWidget(radioGlobal);

        layout.addStretch();
    }
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    // Determine the mode selected by the user
    var mode = "--user";
    if (mnePythonPage) {
        var radioGlobal = mnePythonPage.findChild("radioGlobal");
        if (radioGlobal && radioGlobal.checked) {
            mode = "--global";
        }
    }

    var targetDir = installer.value("TargetDir");

    if (systemInfo.kernelType === "winnt") {
        // On Windows, global install just runs in the current (possibly
        // elevated) prompt; --user adds the --user pip flag.
        component.addOperation("Execute",
            "cmd.exe", "/C",
            targetDir + "\\scripts\\install_mne_python.bat",
            mode,
            "workingdirectory=" + targetDir);
    } else {
        component.addOperation("Execute",
            "/bin/bash",
            targetDir + "/scripts/install_mne_python.sh",
            mode,
            "workingdirectory=" + targetDir);
    }
}
