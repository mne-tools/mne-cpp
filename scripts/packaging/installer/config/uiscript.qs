// uiscript.qs — Interactive installer UI enhancements
//
// This controller script customises the wizard pages for interactive use.
// It does NOT auto-click any buttons; the user controls navigation.
// (For headless installs, pass controlscript.qs via the --script flag instead.)

function Controller()
{
    // Show a welcoming status bar message
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
}

// ---------------------------------------------------------------------------
//  Introduction page
// ---------------------------------------------------------------------------
Controller.prototype.IntroductionPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        page.description =
            "This wizard will guide you through the installation of " +
            "<b>MNE-CPP 2.0</b>.<br><br>" +
            "MNE-CPP is a cross-platform, open-source C++ toolkit for " +
            "MEG and EEG data acquisition, processing, and visualization.<br><br>" +
            "Click <b>Next</b> to continue.";
    }
}

// ---------------------------------------------------------------------------
//  Target directory page
// ---------------------------------------------------------------------------
Controller.prototype.TargetDirectoryPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        page.description =
            "Select the directory where MNE-CPP will be installed.<br><br>" +
            "The default location is <tt>~/MNE-CPP</tt>, similar to how " +
            "Qt installs under <tt>~/Qt</tt>. Applications, libraries, " +
            "headers, and data will be placed inside this directory.";
    }
}

// ---------------------------------------------------------------------------
//  Component selection page
// ---------------------------------------------------------------------------
Controller.prototype.ComponentSelectionPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        page.description =
            "Choose which MNE-CPP components to install. " +
            "Hover over a component to see its description.<br><br>" +
            "<b>Applications</b> includes the core tools. " +
            "Optional components like the <b>Development SDK</b>, " +
            "<b>Sample Dataset</b>, and <b>MNE-Python</b> can be " +
            "added or removed later using the Maintenance Tool.";
    }
}

// ---------------------------------------------------------------------------
//  Installation progress page  –  auto-expand the details log
// ---------------------------------------------------------------------------
Controller.prototype.PerformInstallationPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        // Automatically show the detailed installation log so the user
        // can see exactly what is being installed / downloaded.
        page.showDetails();
    }
}

// ---------------------------------------------------------------------------
//  Finished page
// ---------------------------------------------------------------------------
Controller.prototype.FinishedPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        page.description =
            "MNE-CPP has been installed successfully.<br>" +
            "Visit <a href=\"https://mne-cpp.github.io/\">mne-cpp.github.io</a> " +
            "for documentation and tutorials.";
    }
}
