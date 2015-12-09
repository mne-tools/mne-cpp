function Component()
{
    // default constructor
}


//*************************************************************************************************************

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}


//*************************************************************************************************************

Component.prototype.createOperations = function()
{
    try {
        component.createOperations();

        // create mne_browse_raw_qt shortcut
        if (installer.value("os") === "win") {
            component.addOperation("CreateShortcut", "@TargetDir@/mne_browse_raw_qt.exe", "@StartMenuDir@/MNE-CPP/mne_browse_raw_qt.exe.lnk");
        }
    } catch (e) {
        print(e);
    }
}
