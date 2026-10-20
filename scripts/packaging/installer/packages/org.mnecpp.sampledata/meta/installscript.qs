// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Downloads the MNE sample dataset into <install dir>/data/MNE-sample-data.
// curl and tar ship with macOS, Linux and Windows 10 or newer.

var url = "https://osf.io/86qa2/download";

function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    var data = installer.value("TargetDir") + "/data";
    var failed = "errormessage=The MNE sample dataset could not be downloaded. Check the internet connection, " +
                 "or add it later with the MNE-CPP Maintenance Tool.";

    component.addOperation("Mkdir", data);
    if (systemInfo.kernelType === "winnt") {
        var winData = installer.toNativeSeparators(data);
        var archive = winData + "\\MNE-sample-data.tar.gz";
        component.addOperation("Execute", "cmd.exe", "/C",
            "curl.exe -fL --retry 3 -o \"" + archive + "\" " + url + " && tar.exe -xzf \"" + archive +
            "\" -C \"" + winData + "\" && del /Q \"" + archive + "\"",
            failed,
            "UNDOEXECUTE", "cmd.exe", "/C", "rmdir /S /Q \"" + winData + "\\MNE-sample-data\"");
        component.addOperation("EnvironmentVariable", "MNE_DATASETS_SAMPLE_PATH", winData, "true");
    } else {
        var tarball = data + "/MNE-sample-data.tar.gz";
        component.addOperation("Execute", "/bin/sh", "-c",
            "curl -fL --retry 3 -o \"" + tarball + "\" " + url + " && tar -xzf \"" + tarball + "\" -C \"" + data +
            "\" && rm -f \"" + tarball + "\"",
            failed,
            "UNDOEXECUTE", "/bin/rm", "-rf", data + "/MNE-sample-data");
    }
}
