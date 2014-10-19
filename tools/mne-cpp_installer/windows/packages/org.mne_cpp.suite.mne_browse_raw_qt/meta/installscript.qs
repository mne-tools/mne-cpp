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
        component.addOperation("CreateShortcut", "@TargetDir@/mne_browse_raw_qt/readme.txt", "@StartMenuDir@/MNE Browse Raw Qt/README.lnk",
            "workingDirectory=@TargetDir@", "iconPath=%SystemRoot%/system32/SHELL32.dll",
            "iconId=2");
    }
}
