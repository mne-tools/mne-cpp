// installscript.qs — GUI Applications post-install hook
//
// On macOS: creates symlinks in /Applications/MNE-CPP/ so that GUI apps
// appear in Finder, Launchpad, and Spotlight like native applications.
// On Windows: creates Start Menu shortcuts.
// On Linux: installs .desktop files for desktop integration.

var GUI_APPS = ["mne_scan", "mne_analyze", "mne_browse", "mne_inspect"];

function Component()
{
    // nothing to do at construction time
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    var targetDir = installer.value("TargetDir");

    if (systemInfo.productType === "osx") {
        // -----------------------------------------------------------------
        //  macOS: symlink .app bundles into /Applications/MNE-CPP/
        // -----------------------------------------------------------------
        var appsFolder = "/Applications/MNE-CPP";

        component.addOperation("Execute",
            "/bin/mkdir", "-p", appsFolder,
            "UNDOEXECUTE",
            "/bin/rm", "-rf", appsFolder);

        for (var i = 0; i < GUI_APPS.length; i++) {
            var app = GUI_APPS[i];
            var source = targetDir + "/bin/" + app + ".app";
            var link   = appsFolder + "/" + app + ".app";

            component.addOperation("Execute",
                "/bin/ln", "-sf", source, link,
                "UNDOEXECUTE",
                "/bin/rm", "-f", link);
        }

    } else if (systemInfo.kernelType === "winnt") {
        // -----------------------------------------------------------------
        //  Windows: create Start Menu shortcuts
        // -----------------------------------------------------------------
        var startMenu = installer.value("StartMenuDir");

        for (var i = 0; i < GUI_APPS.length; i++) {
            var app = GUI_APPS[i];
            // Pretty name: mne_scan → MNE Scan
            var prettyName = app.replace("mne_", "MNE ").replace(/\b\w/g,
                function(c) { return c.toUpperCase(); });

            component.addOperation("CreateShortcut",
                targetDir + "\\bin\\" + app + ".exe",
                "@StartMenuDir@/" + prettyName + ".lnk",
                "workingDirectory=" + targetDir + "\\bin",
                "description=Launch " + prettyName);
        }

    } else {
        // -----------------------------------------------------------------
        //  Linux: install .desktop files into ~/.local/share/applications/
        // -----------------------------------------------------------------
        var desktopDir = installer.environmentVariable("HOME") +
                         "/.local/share/applications";

        component.addOperation("Mkdir", desktopDir);

        for (var i = 0; i < GUI_APPS.length; i++) {
            var app = GUI_APPS[i];
            var prettyName = app.replace("mne_", "MNE ").replace(/\b\w/g,
                function(c) { return c.toUpperCase(); });

            var desktop =
                "[Desktop Entry]\n" +
                "Type=Application\n" +
                "Name=" + prettyName + "\n" +
                "Exec=" + targetDir + "/bin/" + app + "\n" +
                "Icon=" + targetDir + "/bin/" + app + "\n" +
                "Terminal=false\n" +
                "Categories=Science;Education;\n" +
                "Comment=MNE-CPP " + prettyName + "\n";

            var desktopFile = desktopDir + "/mnecpp-" + app + ".desktop";

            component.addOperation("Execute",
                "/bin/bash", "-c",
                "echo '" + desktop + "' > '" + desktopFile + "' && " +
                "chmod +x '" + desktopFile + "'",
                "UNDOEXECUTE",
                "/bin/rm", "-f", desktopFile);
        }
    }
}
