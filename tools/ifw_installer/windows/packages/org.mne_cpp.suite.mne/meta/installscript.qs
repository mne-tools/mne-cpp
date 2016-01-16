function Component()
{
    // default constructor
}


//*************************************************************************************************************

Component.prototype.createOperations = function()
{

//    try {
        component.createOperations();

        // create mne library shortcuts
        if (installer.value("os") === "win") {
//        if (systemInfo.productType === "windows") {
            component.addOperation("CreateShortcut", "@TargetDir@/MNECppMaintenanceTool.exe", "@StartMenuDir@/MNE-CPP/MNECppMaintenanceTool.exe.lnk", "workingDirectory=@TargetDir@");//"iconPath=@TargetDir@/App.ico");
        }
//    } catch (e) {
//        print(e);
//    }
}
