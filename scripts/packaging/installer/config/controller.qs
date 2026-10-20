// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026 MNE-CPP Authors
//   Christoph Dinh <christoph.dinh@mne-cpp.org>
//
// Titles and wording of the installer pages. The setup type and finish page logic
// lives in packages/org.mnecpp.runtime/meta/installscript.qs.

function Controller()
{
    // Shown only for a custom setup; hidden before the components load so the page list starts right
    installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
    installer.titleMessageChanged.connect(function(title) {
        var page = gui.pageById(QInstaller.PerformInstallation);
        if (page !== null && gui.currentPageWidget() === page) {
            page.title = styledTitle(title);
        }
    });
}

function styledTitle(title)
{
    return "<span style='font-size:20px; font-weight:600'>" + title + "</span>";
}

// The page list shows the plain text of the title, the header shows it styled
function setHeader(title, subTitle)
{
    var page = gui.currentPageWidget();
    if (page === null) {
        return;
    }
    page.title = styledTitle(title);
    page.subTitle = "<span style='color:#7a7f87'>" + subTitle + "</span>";
}

function feature(title, text, padding)
{
    return "<td width='33%' style='padding:" + padding + "'>" +
           "<span style='color:#6c5ce7; font-size:11px; font-weight:700; letter-spacing:1px'>" + title + "</span><br/>" +
           "<span style='color:#4a4a68; line-height:140%'>" + text + "</span></td>";
}

Controller.prototype.IntroductionPageCallback = function()
{
    if (!installer.isInstaller()) {
        return;
    }
    setHeader("Welcome", "MNE-CPP @MNE_CPP_VERSION@ · installs into your home folder, no administrator rights needed");
    gui.currentPageWidget().MessageLabel.setText(
        // Qt rich text ignores block and table margins here, so cell padding does the spacing
        "<table width='100%' cellspacing='0'><tr><td colspan='3' style='padding:28px 0 0 0'>" +
        "<span style='font-size:26px; font-weight:600; color:#1a1a2e'>Real-time brain imaging in C++</span></td></tr>" +
        "<tr><td colspan='3' style='padding:8px 0 0 0'>" +
        "<span style='font-size:15px; color:#4a4a68'>The open-source framework for MEG, EEG and BCI.</span></td></tr><tr>" +
        feature("ACQUIRE", "Real-time MEG, EEG and BCI pipelines in MNE Scan", "44px 20px 0 0") +
        feature("ANALYSE", "Browse, filter and localise sources in 3-D", "44px 10px 0 10px") +
        feature("BUILD", "C++ libraries and a CMake SDK for your own tools", "44px 0 0 20px") +
        "</tr></table>");
}

Controller.prototype.ComponentSelectionPageCallback = function()
{
    setHeader("Select Components", "Pick the parts of MNE-CPP you need.");
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    setHeader("Installation Folder", "Choose where and what to install. The Maintenance Tool changes it later.");
}

Controller.prototype.LicenseAgreementPageCallback = function()
{
    setHeader("License Agreement", "MNE-CPP is open source under the BSD 3-Clause License.");
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    setHeader("Installation Summary", "Everything is ready. Click Install to begin.");
}

Controller.prototype.PerformInstallationPageCallback = function()
{
    setHeader("Installing", "This takes a minute or two.");
}

Controller.prototype.FinishedPageCallback = function()
{
    if (!installer.isInstaller() || installer.status !== QInstaller.Success) {
        return;
    }
    setHeader("Finished", "MNE-CPP @MNE_CPP_VERSION@ is installed in " +
              installer.toNativeSeparators(installer.value("TargetDir")) + ".");
    // The header already names the folder; the finish widget replaces the stock hints
    var page = gui.currentPageWidget();
    page.MessageLabel.setText("");
    page.FinishText.visible = false;
    if (page.LocationLabel) {
        page.LocationLabel.visible = false;
    }
}
