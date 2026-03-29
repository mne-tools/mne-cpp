// installscript.qs -- Core MNE-CPP binaries
//
// This component turns the installer into a lightweight bootstrapper:
// users choose a dynamic or static package, a dedicated page downloads
// the matching archive with live progress, and the standard installer
// page then shows extraction/configuration progress.

var binaryVariantPage = null;
var downloadProgressPage = null;
var downloadProgressBar = null;
var downloadPercentLabel = null;
var downloadMessageLabel = null;
var downloadDetailLabel = null;
var downloadSummaryLabel = null;
var downloadStepDownload = null;
var downloadStepInstall = null;
var downloadStepFinish = null;
var downloadTimer = null;

var releaseTag = "@MNE_CPP_RELEASE_TAG@";
var assetPrefix = "@MNE_CPP_ASSET_PREFIX@";
var defaultVariant = "dynamic";
var guiApps = ["mne_scan", "mne_analyze", "mne_browse", "mne_inspect"];

var activeVariant = "";
var activeAssetName = "";
var activeAssetUrl = "";
var downloadTempRoot = "";
var downloadArchiveDir = "";
var downloadProgressFile = "";
var downloadArchivePath = "";
var downloadStarted = false;
var downloadFinished = false;
var downloadFailed = false;
var downloadAutoAdvanced = false;

function platformName()
{
    if (systemInfo.productType === "osx") {
        return "macos";
    }

    if (systemInfo.kernelType === "winnt") {
        return "windows";
    }

    return "linux";
}

function pathSeparator()
{
    if (systemInfo.kernelType === "winnt") {
        return "\\";
    }

    return "/";
}

function joinPath(base, leaf)
{
    if (base === "") {
        return leaf;
    }

    var sep = pathSeparator();
    if (base.charAt(base.length - 1) === "/" || base.charAt(base.length - 1) === "\\") {
        return base + leaf;
    }

    return base + sep + leaf;
}

function tempBaseDirectory()
{
    var candidates = [];

    if (systemInfo.kernelType === "winnt") {
        candidates = ["TEMP", "TMP"];
    } else {
        candidates = ["TMPDIR", "TEMP", "TMP"];
    }

    for (var i = 0; i < candidates.length; ++i) {
        var value = installer.environmentVariable(candidates[i]);
        if (value !== "") {
            return installer.fromNativeSeparators(value);
        }
    }

    return installer.value("TargetDir");
}

function shellQuote(value)
{
    return "'" + String(value).replace(/'/g, "'\"'\"'") + "'";
}

function powerShellQuote(value)
{
    return "'" + String(value).replace(/'/g, "''") + "'";
}

function parseIntSafe(value, fallback)
{
    var parsed = parseInt(value, 10);
    if (isNaN(parsed)) {
        return fallback;
    }

    return parsed;
}

function selectedVariant()
{
    var envVariant = installer.environmentVariable("MNE_CPP_VARIANT");
    if (envVariant === "dynamic" || envVariant === "static") {
        return envVariant;
    }

    if (binaryVariantPage) {
        var radioStatic = binaryVariantPage.findChild("radioStatic");
        if (radioStatic && radioStatic.checked) {
            return "static";
        }
    }

    return defaultVariant;
}

function prettyVariantName(variant)
{
    if (variant === "static") {
        return "Static";
    }

    return "Dynamic";
}

function releaseAssetName(variant)
{
    var platform = platformName();

    if (platform === "windows") {
        return assetPrefix + "-windows-" + variant + "-x86_64.zip";
    }

    if (platform === "macos") {
        return assetPrefix + "-macos-" + variant + "-arm64.tar.gz";
    }

    return assetPrefix + "-linux-" + variant + "-x86_64.tar.gz";
}

function releaseAssetUrl(variant)
{
    return "https://github.com/mne-tools/mne-cpp/releases/download/" +
           releaseTag + "/" + releaseAssetName(variant);
}

function shouldSkipDesktopIntegration()
{
    return installer.environmentVariable("MNE_CPP_CLI_ONLY") === "1";
}

function readKeyValueFile(filePath)
{
    var result = {};

    if (!installer.fileExists(filePath)) {
        return result;
    }

    var content = installer.readFile(filePath, "UTF-8");
    if (content === "") {
        return result;
    }

    var lines = content.split(/\r?\n/);
    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i];
        var index = line.indexOf("=");
        if (index <= 0) {
            continue;
        }

        var key = line.substring(0, index);
        var value = line.substring(index + 1);
        result[key] = value;
    }

    return result;
}

function setStepState(label, state)
{
    if (!label) {
        return;
    }

    var prefix = "\u25cb";
    if (state === "active") {
        prefix = "\u25b6";
    } else if (state === "done") {
        prefix = "\u2713";
    } else if (state === "error") {
        prefix = "\u2715";
    }

    var baseText = label.property("baseText");
    if (!baseText) {
        baseText = label.text;
        label.setProperty("baseText", baseText);
    }

    label.text = prefix + " " + baseText;
    label.setProperty("state", state);
    label.styleSheet = "";
}

function updateDownloadVisuals(progress, message, detail, state)
{
    if (!downloadProgressPage) {
        return;
    }

    downloadProgressPage.complete = downloadFinished && !downloadFailed;

    if (downloadProgressBar) {
        downloadProgressBar.value = progress;
    }

    if (downloadPercentLabel) {
        downloadPercentLabel.text = progress + "%";
    }

    if (downloadMessageLabel) {
        downloadMessageLabel.text = message;
    }

    if (downloadDetailLabel) {
        downloadDetailLabel.text = detail;
    }

    if (downloadSummaryLabel) {
        downloadSummaryLabel.text =
            prettyVariantName(activeVariant) + " package for " + platformName() +
            " from release " + releaseTag;
    }

    if (state === "downloading") {
        setStepState(downloadStepDownload, "active");
        setStepState(downloadStepInstall, "pending");
        setStepState(downloadStepFinish, "pending");
    } else if (state === "downloaded") {
        setStepState(downloadStepDownload, "done");
        setStepState(downloadStepInstall, "active");
        setStepState(downloadStepFinish, "pending");
    } else if (state === "ready") {
        setStepState(downloadStepDownload, "done");
        setStepState(downloadStepInstall, "done");
        setStepState(downloadStepFinish, "active");
    } else if (state === "error") {
        setStepState(downloadStepDownload, "error");
        setStepState(downloadStepInstall, "pending");
        setStepState(downloadStepFinish, "pending");
    } else {
        setStepState(downloadStepDownload, "pending");
        setStepState(downloadStepInstall, "pending");
        setStepState(downloadStepFinish, "pending");
    }
}

function stopDownloadTimer()
{
    if (downloadTimer) {
        downloadTimer.stop();
    }
}

function ensureDownloadTimer()
{
    if (downloadTimer || !downloadProgressPage) {
        return;
    }

    downloadTimer = new QTimer(downloadProgressPage);
    downloadTimer.setInterval(250);
    downloadTimer.timeout.connect(function() {
        pollDownloadProgress();
    });
}

function pollDownloadProgress()
{
    if (downloadProgressFile === "") {
        return;
    }

    var state = readKeyValueFile(downloadProgressFile);
    if (!state.STATE) {
        return;
    }

    var progress = parseIntSafe(state.PROGRESS, 0);
    var message = state.MESSAGE || "Preparing download";
    var detail = activeAssetName;

    if (state.STATE === "downloading") {
        updateDownloadVisuals(progress, message, detail, "downloading");
        return;
    }

    if (state.STATE === "downloaded") {
        updateDownloadVisuals(100, "Download complete", activeAssetName, "downloaded");
        if (state.ARCHIVE_PATH) {
            downloadArchivePath = state.ARCHIVE_PATH;
        }
        return;
    }

    if (state.STATE === "ready") {
        stopDownloadTimer();
        downloadFinished = true;
        downloadFailed = false;
        if (state.ARCHIVE_PATH) {
            downloadArchivePath = state.ARCHIVE_PATH;
        }
        installer.setValue("MNECPP_ARCHIVE_PATH", downloadArchivePath);
        installer.setValue("MNECPP_SELECTED_VARIANT", activeVariant);
        installer.setValue("MNECPP_RELEASE_TAG", releaseTag);
        updateDownloadVisuals(100,
            "Download complete. Starting installation...",
            activeAssetName,
            "ready");

        if (!downloadAutoAdvanced) {
            downloadAutoAdvanced = true;
            gui.clickButton(buttons.NextButton);
        }
        return;
    }

    if (state.STATE === "error") {
        stopDownloadTimer();
        downloadFailed = true;
        downloadFinished = false;
        updateDownloadVisuals(progress,
            "Download failed",
            state.ERROR || state.MESSAGE || "Unknown error",
            "error");
    }
}

function detachedShellDownloadCommand(url, archiveDir, progressFile, assetName)
{
    var archivePath = joinPath(archiveDir, assetName);
    var script =
        "set -euo pipefail\n" +
        "archive_dir=" + shellQuote(archiveDir) + "\n" +
        "progress_file=" + shellQuote(progressFile) + "\n" +
        "archive_path=" + shellQuote(archivePath) + "\n" +
        "url=" + shellQuote(url) + "\n" +
        "success=0\n" +
        "write_state() {\n" +
        "  state=\"$1\"\n" +
        "  progress=\"$2\"\n" +
        "  message=\"$3\"\n" +
        "  done=\"$4\"\n" +
        "  error=\"$5\"\n" +
        "  tmp_file=\"${progress_file}.tmp\"\n" +
        "  cat > \"$tmp_file\" <<EOF\n" +
        "STATE=${state}\n" +
        "PROGRESS=${progress}\n" +
        "MESSAGE=${message}\n" +
        "ARCHIVE_PATH=${archive_path}\n" +
        "DONE=${done}\n" +
        "ERROR=${error}\n" +
        "EOF\n" +
        "  mv \"$tmp_file\" \"$progress_file\"\n" +
        "}\n" +
        "trap 'rc=$?; if [ \"$success\" -ne 1 ]; then write_state error 0 \"Download failed\" 1 \"exit ${rc}\"; fi' EXIT\n" +
        "mkdir -p \"$archive_dir\"\n" +
        "write_state downloading 0 \"Preparing download\" 0 \"\"\n" +
        "if command -v curl >/dev/null 2>&1; then\n" +
        "  curl -L --fail -o \"$archive_path\" \"$url\" --progress-bar 2>&1 |\n" +
        "    while IFS= read -r -d $'\\r' line; do\n" +
        "      pct=$(printf '%s\\n' \"$line\" | sed -n 's/.*\\([0-9][0-9]*\\.?[0-9]*\\)%.*/\\1/p' | tail -n 1)\n" +
        "      if [ -n \"$pct\" ]; then\n" +
        "        pct=${pct%%.*}\n" +
        "        write_state downloading \"$pct\" \"Downloading release archive\" 0 \"\"\n" +
        "      fi\n" +
        "    done\n" +
        "elif command -v wget >/dev/null 2>&1; then\n" +
        "  wget --progress=bar:force:noscroll -O \"$archive_path\" \"$url\" 2>&1 |\n" +
        "    while IFS= read -r line; do\n" +
        "      pct=$(printf '%s\\n' \"$line\" | sed -n 's/.* \\([0-9][0-9]*\\)%.*/\\1/p' | tail -n 1)\n" +
        "      if [ -n \"$pct\" ]; then\n" +
        "        write_state downloading \"$pct\" \"Downloading release archive\" 0 \"\"\n" +
        "      fi\n" +
        "    done\n" +
        "else\n" +
        "  write_state error 0 \"Download failed\" 1 \"Neither curl nor wget is available\"\n" +
        "  exit 1\n" +
        "fi\n" +
        "write_state downloaded 100 \"Download complete\" 1 \"\"\n" +
        "write_state ready 100 \"Ready to install\" 1 \"\"\n" +
        "success=1\n";

    return "cat > " + shellQuote(joinPath(downloadTempRoot, "download.sh")) + " <<'EOF'\n" +
           script +
           "EOF\n" +
           "chmod +x " + shellQuote(joinPath(downloadTempRoot, "download.sh")) + "\n" +
           shellQuote(joinPath(downloadTempRoot, "download.sh"));
}

function detachedPowerShellCommand(url, archiveDir, progressFile, assetName)
{
    var archivePath = joinPath(archiveDir, assetName);
    return "$ErrorActionPreference = 'Stop'; " +
           "$archiveDir = " + powerShellQuote(installer.toNativeSeparators(archiveDir)) + "; " +
           "$progressFile = " + powerShellQuote(installer.toNativeSeparators(progressFile)) + "; " +
           "$archivePath = " + powerShellQuote(installer.toNativeSeparators(archivePath)) + "; " +
           "$url = " + powerShellQuote(url) + "; " +
           "function Write-State($state, $progress, $message, $done, $error) { " +
           "  $tmp = $progressFile + '.tmp'; " +
           "  $content = @(\"STATE=$state\", \"PROGRESS=$progress\", \"MESSAGE=$message\", \"ARCHIVE_PATH=$archivePath\", \"DONE=$done\", \"ERROR=$error\"); " +
           "  Set-Content -Path $tmp -Value $content -Encoding UTF8; " +
           "  Move-Item -Path $tmp -Destination $progressFile -Force; " +
           "} " +
           "New-Item -ItemType Directory -Force -Path $archiveDir | Out-Null; " +
           "Write-State 'downloading' 0 'Preparing download' 0 ''; " +
           "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " +
           "$wc = New-Object System.Net.WebClient; " +
           "$wc.DownloadProgressChanged += { param($sender, $e) Write-State 'downloading' $e.ProgressPercentage 'Downloading release archive' 0 '' }; " +
           "$wc.DownloadFileCompleted += { param($sender, $e) " +
           "  if ($e.Error) { Write-State 'error' 0 'Download failed' 1 $e.Error.Message } " +
           "  elseif ($e.Cancelled) { Write-State 'error' 0 'Download canceled' 1 'canceled' } " +
           "  else { Write-State 'downloaded' 100 'Download complete' 1 ''; Write-State 'ready' 100 'Ready to install' 1 '' } " +
           "}; " +
           "$wc.DownloadFileAsync((New-Object Uri($url)), $archivePath); " +
           "while ($wc.IsBusy) { Start-Sleep -Milliseconds 200 }";
}

function beginDetachedDownload()
{
    var targetDir = installer.value("TargetDir");
    activeVariant = selectedVariant();
    activeAssetName = releaseAssetName(activeVariant);
    activeAssetUrl = releaseAssetUrl(activeVariant);
    downloadTempRoot = joinPath(tempBaseDirectory(),
        "mnecpp-bootstrap-" + activeVariant + "-" + new Date().getTime());
    downloadArchiveDir = joinPath(downloadTempRoot, "archives");
    downloadProgressFile = joinPath(downloadTempRoot, "progress.txt");
    downloadArchivePath = joinPath(downloadArchiveDir, activeAssetName);
    downloadStarted = true;
    downloadFinished = false;
    downloadFailed = false;
    downloadAutoAdvanced = false;

    installer.setValue("MNECPP_SELECTED_VARIANT", activeVariant);
    installer.setValue("MNECPP_RELEASE_TAG", releaseTag);
    installer.setValue("MNECPP_ARCHIVE_PATH", "");

    updateDownloadVisuals(0,
        "Preparing secure download",
        prettyVariantName(activeVariant) + " package from " + releaseTag,
        "pending");

    ensureDownloadTimer();
    if (downloadTimer) {
        downloadTimer.start();
    }

    var started = false;
    if (systemInfo.kernelType === "winnt") {
        started = installer.executeDetached("powershell", [
            "-ExecutionPolicy", "Bypass",
            "-NoProfile",
            "-Command", detachedPowerShellCommand(activeAssetUrl, downloadArchiveDir, downloadProgressFile, activeAssetName)
        ], targetDir);
    } else {
        started = installer.executeDetached("/bin/bash", [
            "-lc",
            detachedShellDownloadCommand(activeAssetUrl, downloadArchiveDir, downloadProgressFile, activeAssetName)
        ], targetDir);
    }

    if (!started) {
        stopDownloadTimer();
        downloadFailed = true;
        updateDownloadVisuals(0,
            "Download could not be started",
            "The installer could not launch the background download process.",
            "error");
    }
}

function ensureVariantPage()
{
    installer.addWizardPage(component, "BinaryVariantPage", QInstaller.TargetDirectory);
    binaryVariantPage = gui.pageWidgetByObjectName("DynamicBinaryVariantPage");
    if (!binaryVariantPage) {
        return;
    }

    binaryVariantPage.title = "Binary Variant";
    binaryVariantPage.description =
        "Pick the package you want the installer to fetch from GitHub Releases.";

    var rootLayout = new QVBoxLayout(binaryVariantPage);
    rootLayout.setSpacing(14);

    var card = new QFrame(binaryVariantPage);
    card.objectName = "MneHeroCard";
    var cardLayout = new QVBoxLayout(card);
    cardLayout.setSpacing(10);

    var kicker = new QLabel("Step 1 of 2", card);
    kicker.objectName = "MneKicker";
    cardLayout.addWidget(kicker);

    var title = new QLabel("Choose your MNE-CPP binary flavor", card);
    title.objectName = "MneTitle";
    cardLayout.addWidget(title);

    var body = new QLabel(
        "The installer stays lightweight and downloads the matching archive for " +
        platformName() + " from release " + releaseTag + ".",
        card
    );
    body.objectName = "MneBody";
    cardLayout.addWidget(body);

    var optionCard = new QFrame(card);
    optionCard.objectName = "MneOptionPanel";
    var optionLayout = new QVBoxLayout(optionCard);
    optionLayout.setSpacing(12);

    var radioDynamic = new QRadioButton("Dynamic build", optionCard);
    radioDynamic.objectName = "radioDynamic";
    radioDynamic.checked = true;
    optionLayout.addWidget(radioDynamic);

    var dynamicNote = new QLabel(
        "Shared libraries, smaller download, full plugin support. Best default for most users.",
        optionCard
    );
    dynamicNote.objectName = "MneDetail";
    optionLayout.addWidget(dynamicNote);

    var radioStatic = new QRadioButton("Static build", optionCard);
    radioStatic.objectName = "radioStatic";
    optionLayout.addWidget(radioStatic);

    var staticNote = new QLabel(
        "More self-contained binaries with a larger download size. Useful when you want fewer runtime dependencies.",
        optionCard
    );
    staticNote.objectName = "MneDetail";
    optionLayout.addWidget(staticNote);

    cardLayout.addWidget(optionCard);
    rootLayout.addWidget(card);
    rootLayout.addStretch();
}

function ensureDownloadPage()
{
    installer.addWizardPage(component, "DownloadProgressPage", QInstaller.PerformInstallation);
    downloadProgressPage = gui.pageWidgetByObjectName("DynamicDownloadProgressPage");
    if (!downloadProgressPage) {
        return;
    }

    downloadProgressPage.title = "Download Core Binaries";
    downloadProgressPage.description =
        "Fetching the selected MNE-CPP archive before installation begins.";
    downloadProgressPage.complete = false;

    var rootLayout = new QVBoxLayout(downloadProgressPage);
    rootLayout.setSpacing(14);

    var card = new QFrame(downloadProgressPage);
    card.objectName = "MneHeroCard";
    var cardLayout = new QVBoxLayout(card);
    cardLayout.setSpacing(12);

    var kicker = new QLabel("Step 2 of 2", card);
    kicker.objectName = "MneKicker";
    cardLayout.addWidget(kicker);

    var title = new QLabel("Downloading the selected release archive", card);
    title.objectName = "MneTitle";
    cardLayout.addWidget(title);

    downloadSummaryLabel = new QLabel("", card);
    downloadSummaryLabel.objectName = "MneBody";
    cardLayout.addWidget(downloadSummaryLabel);

    var progressPanel = new QFrame(card);
    progressPanel.objectName = "MneOptionPanel";
    var progressLayout = new QVBoxLayout(progressPanel);
    progressLayout.setSpacing(10);

    var progressHeader = new QHBoxLayout();
    downloadMessageLabel = new QLabel("Waiting to start...", progressPanel);
    downloadMessageLabel.objectName = "MneStatusText";
    progressHeader.addWidget(downloadMessageLabel);
    progressHeader.addStretch();
    downloadPercentLabel = new QLabel("0%", progressPanel);
    downloadPercentLabel.objectName = "MneMetric";
    progressHeader.addWidget(downloadPercentLabel);
    progressLayout.addLayout(progressHeader);

    downloadProgressBar = new QProgressBar(progressPanel);
    downloadProgressBar.objectName = "MneDownloadProgress";
    downloadProgressBar.minimum = 0;
    downloadProgressBar.maximum = 100;
    downloadProgressBar.value = 0;
    progressLayout.addWidget(downloadProgressBar);

    downloadDetailLabel = new QLabel("Preparing secure download...", progressPanel);
    downloadDetailLabel.objectName = "MneDetail";
    progressLayout.addWidget(downloadDetailLabel);

    cardLayout.addWidget(progressPanel);

    var checklist = new QFrame(card);
    checklist.objectName = "MneOptionPanel";
    var checklistLayout = new QVBoxLayout(checklist);
    checklistLayout.setSpacing(8);

    downloadStepDownload = new QLabel("Download release archive", checklist);
    downloadStepDownload.objectName = "MneStepItem";
    checklistLayout.addWidget(downloadStepDownload);

    downloadStepInstall = new QLabel("Extract binaries into the target directory", checklist);
    downloadStepInstall.objectName = "MneStepItem";
    checklistLayout.addWidget(downloadStepInstall);

    downloadStepFinish = new QLabel("Configure shortcuts and optional tools", checklist);
    downloadStepFinish.objectName = "MneStepItem";
    checklistLayout.addWidget(downloadStepFinish);

    cardLayout.addWidget(checklist);
    rootLayout.addWidget(card);
    rootLayout.addStretch();

    updateDownloadVisuals(0, "Waiting to start...", "No download in progress yet.", "pending");
}

function maybeStartDownload()
{
    var variant = selectedVariant();
    var assetName = releaseAssetName(variant);

    if (downloadFinished && !downloadFailed &&
        variant === activeVariant && assetName === activeAssetName &&
        installer.fileExists(downloadArchivePath)) {
        updateDownloadVisuals(100,
            "Download already prepared",
            activeAssetName,
            "ready");
        downloadProgressPage.complete = true;
        gui.clickButton(buttons.NextButton);
        return;
    }

    if (downloadStarted && !downloadFailed &&
        variant === activeVariant && assetName === activeAssetName) {
        ensureDownloadTimer();
        if (downloadTimer) {
            downloadTimer.start();
        }
        return;
    }

    beginDetachedDownload();
}

function Component()
{
    installer.setValue("MNECPP_RELEASE_TAG", releaseTag);
    ensureVariantPage();
    ensureDownloadPage();
    installer.setValidatorForCustomPage(component,
        "DownloadProgressPage",
        "validateDownloadProgressPage");
}

Component.prototype.DynamicDownloadProgressPageCallback = function()
{
    maybeStartDownload();
}

Component.prototype.validateDownloadProgressPage = function()
{
    return downloadFinished && !downloadFailed;
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    var targetDir = installer.value("TargetDir");
    var variant = installer.value("MNECPP_SELECTED_VARIANT", selectedVariant());
    var archivePath = installer.value("MNECPP_ARCHIVE_PATH", "");
    var assetName = releaseAssetName(variant);
    var assetUrl = releaseAssetUrl(variant);

    if (archivePath === "") {
        if (systemInfo.kernelType === "winnt") {
            component.addOperation("Execute",
                "powershell",
                "-ExecutionPolicy", "Bypass",
                "-File", targetDir + "\\scripts\\install_release_archive.ps1",
                "-Url", assetUrl,
                "-TargetDir", targetDir,
                "-AssetName", assetName,
                "workingdirectory=" + targetDir,
                "errormessage=Failed to download or extract the selected MNE-CPP release archive.");
        } else {
            component.addOperation("Execute",
                "/bin/bash",
                targetDir + "/scripts/install_release_archive.sh",
                "--url", assetUrl,
                "--target-dir", targetDir,
                "--asset-name", assetName,
                "workingdirectory=" + targetDir,
                "errormessage=Failed to download or extract the selected MNE-CPP release archive.");
        }
    } else {
        component.addOperation("Extract", archivePath, targetDir);
        if (systemInfo.kernelType === "winnt") {
            var cleanupCommand = "del /Q \"" + installer.toNativeSeparators(archivePath) + "\"";
            if (downloadProgressFile !== "") {
                cleanupCommand += " \"" + installer.toNativeSeparators(downloadProgressFile) + "\"";
            }
            component.addOperation("Execute",
                "cmd.exe", "/C", cleanupCommand,
                "UNDOEXECUTE");
        } else {
            var cleanupScript = "rm -f " + shellQuote(archivePath);
            if (downloadProgressFile !== "") {
                cleanupScript += " " + shellQuote(downloadProgressFile);
            }
            component.addOperation("Execute",
                "/bin/bash", "-c", cleanupScript,
                "UNDOEXECUTE");
        }
    }

    if (shouldSkipDesktopIntegration()) {
        return;
    }

    if (systemInfo.productType === "osx") {
        var appsFolder = "/Applications/MNE-CPP";

        component.addOperation("Execute",
            "/bin/mkdir", "-p", appsFolder,
            "UNDOEXECUTE",
            "/bin/rm", "-rf", appsFolder);

        for (var i = 0; i < guiApps.length; ++i) {
            var macApp = guiApps[i];
            var source = targetDir + "/bin/" + macApp + ".app";
            var link = appsFolder + "/" + macApp + ".app";

            component.addOperation("Execute",
                "/bin/ln", "-sf", source, link,
                "UNDOEXECUTE",
                "/bin/rm", "-f", link);
        }

        return;
    }

    if (systemInfo.kernelType === "winnt") {
        for (var j = 0; j < guiApps.length; ++j) {
            var windowsApp = guiApps[j];
            var windowsPrettyName = windowsApp.replace("mne_", "MNE ").replace(/\b\w/g,
                function(c) { return c.toUpperCase(); });
            component.addOperation("CreateShortcut",
                targetDir + "\\bin\\" + windowsApp + ".exe",
                "@StartMenuDir@/" + windowsPrettyName + ".lnk",
                "workingDirectory=" + targetDir + "\\bin",
                "description=Launch " + prettyVariantName(variant) + " " + windowsPrettyName);
        }

        return;
    }

    var desktopDir = installer.environmentVariable("HOME") + "/.local/share/applications";
    component.addOperation("Mkdir", desktopDir);

    for (var k = 0; k < guiApps.length; ++k) {
        var linuxApp = guiApps[k];
        var prettyName = linuxApp.replace("mne_", "MNE ").replace(/\b\w/g,
            function(c) { return c.toUpperCase(); });
        var desktopFile = desktopDir + "/mnecpp-" + linuxApp + ".desktop";
        var desktop =
            "[Desktop Entry]\n" +
            "Type=Application\n" +
            "Name=" + prettyName + "\n" +
            "Exec=" + targetDir + "/bin/" + linuxApp + "\n" +
            "Icon=" + targetDir + "/bin/" + linuxApp + "\n" +
            "Terminal=false\n" +
            "Categories=Science;Education;\n" +
            "Comment=" + prettyVariantName(variant) + " MNE-CPP " + prettyName + "\n";

        component.addOperation("Execute",
            "/bin/bash", "-c",
            "cat > '" + desktopFile + "' <<'EOF'\n" + desktop + "EOF\nchmod +x '" + desktopFile + "'",
            "UNDOEXECUTE",
            "/bin/rm", "-f", desktopFile);
    }
}
