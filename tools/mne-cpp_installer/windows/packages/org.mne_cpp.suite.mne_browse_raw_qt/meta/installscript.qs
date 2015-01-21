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
    // create mne_browse_raw_qt shortcut
    component.createOperations();

    if (installer.value("os") === "win") {
        component.addOperation("CreateShortcut", "@TargetDir@/mne_browse_raw_qt/mne_browse_raw_qt.exe", "@StartMenuDir@/mne_browse_raw_qt.exe.lnk");
    }
}
