// installscript.qs — MNE Sample Dataset post-install hook
// Executes the download_sample_data script after component installation.

function Component()
{
    // No-op constructor
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    var targetDir = installer.value("TargetDir");

    if (systemInfo.kernelType === "winnt") {
        component.addOperation("Execute",
            "cmd.exe", "/C",
            targetDir + "\\scripts\\download_sample_data.bat",
            "workingdirectory=" + targetDir);
    } else {
        component.addOperation("Execute",
            "/bin/bash",
            targetDir + "/scripts/download_sample_data.sh",
            "workingdirectory=" + targetDir);
    }
}
