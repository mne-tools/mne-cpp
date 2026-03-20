// installscript.qs — PATH Configuration post-install hook
// Executes the configure_environment script after component installation.

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
            targetDir + "\\scripts\\configure_environment.bat",
            "workingdirectory=" + targetDir);
    } else {
        component.addOperation("Execute",
            "/bin/bash",
            targetDir + "/scripts/configure_environment.sh",
            "workingdirectory=" + targetDir);
    }
}
