// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Setup type page and finish page of the graphical installer. They live in the
// always-installed runtime component because only components can add pages.
// Command-line installs choose components by name and skip all of this.

var optionalComponents = ["org.mnecpp.apps", "org.mnecpp.tools", "org.mnecpp.sdk",
                          "org.mnecpp.pathconfig", "org.mnecpp.downloads", "org.mnecpp.sampledata", "org.mnecpp.mnepython"];
var profiles = {
    apps: ["org.mnecpp.apps", "org.mnecpp.tools", "org.mnecpp.pathconfig"],
    developer: ["org.mnecpp.apps", "org.mnecpp.tools", "org.mnecpp.sdk", "org.mnecpp.pathconfig"]
};
var launchers = {
    launchInspect: "mne_inspect",
    launchScan: "mne_scan",
    launchBrowse: "mne_browse",
    launchAnalyze: "mne_analyze"
};

function Component()
{
    if (installer.isCommandLineInstance() || !installer.isInstaller()) {
        return;
    }
    // Like the Qt installer, the setup type sits below the folder, so the page list never changes
    installer.addWizardPageItem(component, "SetupTypePage", QInstaller.TargetDirectory);
    installer.addWizardPageItem(component, "FinishWidget", QInstaller.InstallationFinished);
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);

    var page = component.userInterface("SetupTypePage");
    page.appsProfile.toggled.connect(applySetupType);
    page.developerProfile.toggled.connect(applySetupType);
    page.customProfile.toggled.connect(applySetupType);
    applySetupType();
    installer.installationFinished.connect(prepareFinishWidget);
    installer.finishButtonClicked.connect(runFinishActions);
}

function selectedProfile()
{
    var page = component.userInterface("SetupTypePage");
    if (page.developerProfile.checked) {
        return "developer";
    }
    return page.customProfile.checked ? "custom" : "apps";
}

var summaryRows = [
    ["org.mnecpp.runtime", "Libraries", 0],
    ["org.mnecpp.apps", "Applications", 0],
    ["org.mnecpp.tools", "Command-line tools", 0],
    ["org.mnecpp.pathconfig", "Add to PATH", 1],
    ["org.mnecpp.sdk", "C++ SDK", 0]
];

function componentSize(id)
{
    var c = installer.componentByName(id);
    return c === null ? 0 : parseInt(c.value("UncompressedSize", "0"), 10);
}

function formatSize(bytes)
{
    if (bytes >= 1073741824) {
        return (bytes / 1073741824).toFixed(1) + " GB";
    }
    return Math.max(1, Math.round(bytes / 1048576)) + " MB";
}

function showIncluded()
{
    var label = component.userInterface("SetupTypePage").Included;
    var profile = selectedProfile();
    if (profile === "custom") {
        label.text = "<p><b>What gets installed</b></p>" +
                     "<p style='color:#9aa0a6'>You choose the components on the next page.</p>";
        return;
    }
    var chosen = profiles[profile].concat(["org.mnecpp.runtime"]);
    var total = 0;
    var rows = "";
    for (var i = 0; i < summaryRows.length; ++i) {
        var id = summaryRows[i][0];
        var on = chosen.indexOf(id) >= 0;
        var size = id === "org.mnecpp.pathconfig" ? "" : formatSize(componentSize(id));
        if (on) {
            total += componentSize(id);
        }
        var colour = on ? "" : "color:#9aa0a6;";
        rows += "<tr><td style='" + colour + "padding:2px 10px 2px " + (8 + 18 * summaryRows[i][2]) + "px'>" +
                (on ? "<span style='color:#6c5ce7'>&#10003;</span>" : "&ndash;") + "&nbsp;&nbsp;" + summaryRows[i][1] +
                "</td><td align='right' style='" + colour + "padding:2px 8px'>" + (on ? size : "") + "</td></tr>";
    }
    rows += "<tr><td style='padding:8px 10px 2px 8px'><b>Disk space</b></td>" +
            "<td align='right' style='padding:8px 8px 2px 8px'><b>" + formatSize(total) + "</b></td></tr>";
    label.text = "<p><b>What gets installed</b></p><table width='100%' cellspacing='0'>" + rows + "</table>";
}

function selectOnly(ids)
{
    for (var i = 0; i < optionalComponents.length; ++i) {
        var id = optionalComponents[i];
        if (ids.indexOf(id) >= 0) {
            installer.selectComponent(id);
        } else {
            installer.deselectComponent(id);
        }
    }
}

// Custom starts from the previous profile, so switching back and forth keeps the user's picks
function applySetupType()
{
    var profile = selectedProfile();
    if (profile !== "custom") {
        selectOnly(profiles[profile]);
    }
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, profile === "custom");
    showIncluded();
}

function isInstalled(id)
{
    var c = installer.componentByName(id);
    return c !== null && c.installed;
}

function binDir()
{
    return installer.value("TargetDir") + "/bin/";
}

function prepareFinishWidget()
{
    var widget = component.userInterface("FinishWidget");
    var success = installer.status === QInstaller.Success;
    var apps = success && isInstalled("org.mnecpp.apps");
    var sdk = success && isInstalled("org.mnecpp.sdk");

    widget.visible = success;
    widget.launchTitle.visible = apps;
    for (var name in launchers) {
        widget[name].visible = apps;
    }
    widget.sdkTitle.visible = sdk;
    widget.SdkSnippet.visible = sdk;
    widget.SdkSnippet.plainText =
        "find_package(MNE-CPP REQUIRED)\n" +
        "target_link_libraries(my_app PRIVATE MNE-CPP::mne_fiff MNE-CPP::mne_inv)\n" +
        "cmake -B build -DCMAKE_PREFIX_PATH=\"" + installer.toNativeSeparators(installer.value("TargetDir")) + "\"";
    widget.pathHint.visible = success && isInstalled("org.mnecpp.pathconfig");
}

function launch(app)
{
    var dir = binDir();
    if (systemInfo.productType === "osx") {
        installer.executeDetached("/usr/bin/open", [dir + app + ".app"]);
    } else if (systemInfo.kernelType === "winnt") {
        installer.executeDetached(installer.toNativeSeparators(dir + app + ".exe"), [],
                                  installer.toNativeSeparators(dir));
    } else {
        installer.executeDetached(dir + app, [], dir);
    }
}

function runFinishActions()
{
    if (installer.status !== QInstaller.Success) {
        return;
    }
    var widget = component.userInterface("FinishWidget");
    for (var name in launchers) {
        if (widget[name].visible && widget[name].checked) {
            launch(launchers[name]);
        }
    }
    if (widget.openGuide.checked) {
        QDesktopServices.openUrl("https://mne-cpp.github.io/docs/overview");
    }
    if (widget.openExamples.checked) {
        QDesktopServices.openUrl("https://mne-cpp.github.io/docs/examples");
    }
}
