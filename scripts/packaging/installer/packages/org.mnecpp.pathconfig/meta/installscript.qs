// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Adds bin to the user's PATH on install and removes it again on uninstall.

function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    var scripts = installer.value("TargetDir") + "/scripts/";

    if (systemInfo.kernelType === "winnt") {
        var ps1 = installer.toNativeSeparators(scripts + "configure_environment.ps1");
        component.addOperation("Execute",
            "powershell.exe", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", ps1, "install",
            "UNDOEXECUTE",
            "powershell.exe", "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", ps1, "uninstall");
    } else {
        var sh = scripts + "configure_environment.sh";
        component.addOperation("Execute", "/bin/sh", sh, "install", "UNDOEXECUTE", "/bin/sh", sh, "uninstall");
    }
}
