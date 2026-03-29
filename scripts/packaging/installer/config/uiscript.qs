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
            "This installer uses the Qt Installer Framework online mode, so the " +
            "selected components are downloaded from the MNE-CPP repository during installation.<br><br>" +
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

            var title = new QLabel("Downloading and installing the selected MNE-CPP components", frame);
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
        var repositoryUrl = installer.value("MNECPP_REPOSITORY_URL");
        if (mneInstallSummaryLabel) {
            var summary = "Installing from the online repository";
            if (variant !== "") {
                summary += " using the <b>" + variant + "</b> payload";
            }
            if (releaseTag !== "") {
                summary += " for release channel <b>" + releaseTag + "</b>";
            }
            if (repositoryUrl !== "") {
                summary += ".<br><span style=\"font-size: 11px; color: #576574;\">" + repositoryUrl + "</span>";
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
