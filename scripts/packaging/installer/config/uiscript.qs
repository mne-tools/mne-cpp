// uiscript.qs - Interactive installer UI enhancements
//
// This controller script customises the wizard pages for interactive use.
// It does NOT auto-click any buttons; the user controls navigation.
// (For headless installs, pass controlscript.qs via the --script flag instead.)

var mneInstallPanelInserted = false;
var mneInstallStageLabel = null;
var mneInstallSummaryLabel = null;

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
            "<b>MNE-CPP @MNE_CPP_VERSION@</b>.<br><br>" +
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
            "<b>MNE-CPP Core Binaries</b> downloads the selected platform archive " +
            "from GitHub Releases during installation. Optional components like the " +
            "<b>Sample Dataset</b>, <b>MNE-Python</b>, and <b>Add to PATH</b> can " +
            "be added or removed later using the Maintenance Tool.";
    }
}

// ---------------------------------------------------------------------------
//  Installation progress page - auto-expand the details log
// ---------------------------------------------------------------------------
Controller.prototype.PerformInstallationPageCallback = function()
{
    var page = gui.currentPageWidget();
    if (page) {
        if (!mneInstallPanelInserted && page.layout()) {
            var frame = new QFrame(page);
            frame.objectName = "MneHeroCard";

            var frameLayout = new QVBoxLayout(frame);
            frameLayout.setSpacing(8);

            var kicker = new QLabel("Installation in progress", frame);
            kicker.objectName = "MneKicker";
            frameLayout.addWidget(kicker);

            var title = new QLabel("Step 2 of 2: Extracting and configuring MNE-CPP", frame);
            title.objectName = "MneTitle";
            frameLayout.addWidget(title);

            mneInstallSummaryLabel = new QLabel("", frame);
            mneInstallSummaryLabel.objectName = "MneBody";
            frameLayout.addWidget(mneInstallSummaryLabel);

            mneInstallStageLabel = new QLabel("Preparing installation...", frame);
            mneInstallStageLabel.objectName = "MneStatusText";
            frameLayout.addWidget(mneInstallStageLabel);

            page.layout().insertWidget(0, frame);
            mneInstallPanelInserted = true;
        }

        var variant = installer.value("MNECPP_SELECTED_VARIANT");
        var releaseTag = installer.value("MNECPP_RELEASE_TAG");
        if (mneInstallSummaryLabel) {
            var summary = "Installing the downloaded archive";
            if (variant !== "") {
                summary += " using the <b>" + variant + "</b> package";
            }
            if (releaseTag !== "") {
                summary += " from release <b>" + releaseTag + "</b>.";
            } else {
                summary += ".";
            }
            mneInstallSummaryLabel.text = summary;
        }

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
        var variant = installer.value("MNECPP_SELECTED_VARIANT");
        page.description =
            "MNE-CPP has been installed successfully.<br>" +
            (variant !== "" ? "Installed package: <b>" + variant + "</b><br>" : "") +
            "Visit <a href=\"https://mne-cpp.github.io/\">mne-cpp.github.io</a> " +
            "for documentation and tutorials.";
    }
}

Controller.prototype.onTitleMessageChanged = function(title)
{
    if (mneInstallStageLabel && title !== "") {
        mneInstallStageLabel.text = title;
    }
}
