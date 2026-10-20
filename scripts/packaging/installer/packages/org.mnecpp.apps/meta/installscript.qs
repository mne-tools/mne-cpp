// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Start menu, Applications folder and desktop menu entries for the GUI applications.

var applications = [
    { binary: "mne_scan", name: "MNE Scan", comment: "Real-time MEG/EEG acquisition and processing" },
    { binary: "mne_analyze", name: "MNE Analyze", comment: "Offline MEG/EEG analysis" },
    { binary: "mne_browse", name: "MNE Browse", comment: "Browse raw MEG/EEG recordings" },
    { binary: "mne_inspect", name: "MNE Inspect", comment: "3-D brain surfaces, source estimates and electrodes" },
    { binary: "mne_align", name: "MNE Align", comment: "Coregister digitiser points with the MRI head surface" },
    { binary: "mne_anonymize", name: "MNE Anonymize", comment: "Remove personal information from FIFF files" }
];

function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();
    var target = installer.value("TargetDir");

    if (systemInfo.productType === "osx") {
        // /Applications when the user may write there, ~/Applications otherwise; links only, the bundles stay in bin
        var links = "", unlinks = "";
        for (var i = 0; i < applications.length; ++i) {
            var app = applications[i];
            links += "ln -sfn \"" + target + "/bin/" + app.binary + ".app\" \"$d/" + app.name + ".app\"; ";
            unlinks += "rm -f \"/Applications/MNE-CPP/" + app.name + ".app\" \"$HOME/Applications/MNE-CPP/" + app.name + ".app\"; ";
        }
        component.addOperation("Execute", "/bin/sh", "-c",
            "d=/Applications; [ -w \"$d\" ] || d=\"$HOME/Applications\"; d=\"$d/MNE-CPP\"; mkdir -p \"$d\" && " + links,
            "UNDOEXECUTE", "/bin/sh", "-c",
            unlinks + "rmdir /Applications/MNE-CPP \"$HOME/Applications/MNE-CPP\" 2>/dev/null; true");
        return;
    }

    for (var j = 0; j < applications.length; ++j) {
        var entry = applications[j];
        if (systemInfo.kernelType === "winnt") {
            var exe = installer.toNativeSeparators(target + "/bin/" + entry.binary + ".exe");
            component.addOperation("CreateShortcut", exe, "@StartMenuDir@/" + entry.name + ".lnk",
                "workingDirectory=" + installer.toNativeSeparators(target + "/bin"),
                "description=" + entry.comment);
        } else {
            component.addOperation("CreateDesktopEntry", "mne-cpp-" + entry.binary + ".desktop",
                "Type=Application\nName=" + entry.name + "\nComment=" + entry.comment +
                "\nExec=\"" + target + "/bin/" + entry.binary + "\"\nIcon=" + target + "/resources/icons/" +
                entry.binary + ".png\nTerminal=false\nCategories=Science;MedicalSoftware;Education;");
        }
    }
}
