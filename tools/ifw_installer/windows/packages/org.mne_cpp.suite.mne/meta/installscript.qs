function Component()
{
    // default constructor
}


//*************************************************************************************************************

Component.prototype.createOperations = function()
{

    try {
        component.createOperations();

        // create mne library shortcuts
        if (installer.value("os") === "win") {
            component.addOperation("CreateShortcut", "@TargetDir@/MNECppMaintenanceTool.exe", "@StartMenuDir@/MNE-CPP/MNECppMaintenanceTool.exe.lnk");
        }
    } catch (e) {
        print(e);
    }


}
