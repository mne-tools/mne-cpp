// installscript.qs — MNE Sample Dataset
//
// Features:
//   - Download progress display (curl on macOS/Linux, WebClient on Windows)
//   - Uninstall confirmation (asks before deleting data)

function Component()
{
    // No custom wizard page — data is stored under <TargetDir>/data
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    var targetDir = installer.value("TargetDir");
    var dataDir = targetDir + "/data";
    if (systemInfo.kernelType === "winnt") {
        dataDir = targetDir + "\\data";
    }

    var url = "https://osf.io/86qa2/download";

    if (systemInfo.kernelType === "winnt") {
        // --- Windows ---
        var archive = dataDir + "\\MNE-sample-data.tar.gz";

        // 1. Create data directory
        component.addOperation("Mkdir", dataDir);

        // 2. Download with progress (PowerShell shows percentage)
        component.addOperation("Execute",
            "powershell", "-Command",
            "$ProgressPreference = 'Continue'; " +
            "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; " +
            "Write-Host '=== Downloading MNE Sample Dataset (~1.5 GB) ==='; " +
            "Write-Host 'Source: " + url + "'; " +
            "Write-Host 'Target: " + dataDir + "'; " +
            "Write-Host ''; " +
            "$wc = New-Object System.Net.WebClient; " +
            "$lastPct = -1; " +
            "$wc.add_DownloadProgressChanged({param($sender,$e) " +
            "  if ($e.ProgressPercentage -ne $lastPct -and $e.ProgressPercentage % 5 -eq 0) { " +
            "    $lastPct = $e.ProgressPercentage; " +
            "    $mb = [math]::Round($e.BytesReceived/1MB,1); " +
            "    Write-Host (\"  Progress: {0}%  ({1} MB)\" -f $e.ProgressPercentage, $mb); " +
            "  } " +
            "}); " +
            "$wc.add_DownloadFileCompleted({Write-Host 'Download complete.'}); " +
            "$wc.DownloadFileAsync((New-Object Uri('" + url + "')), '" + archive + "'); " +
            "while ($wc.IsBusy) { Start-Sleep -Milliseconds 500 }",
            "workingdirectory=" + targetDir,
            "errormessage=Failed to download MNE sample dataset. Check your internet connection.");

        // 3. Extract
        component.addOperation("Execute",
            "powershell", "-Command",
            "Write-Host 'Extracting MNE sample dataset...'; " +
            "tar -xzf '" + archive + "' -C '" + dataDir + "'; " +
            "Write-Host 'Extraction complete.'",
            "workingdirectory=" + targetDir,
            "errormessage=Failed to extract MNE sample dataset archive.");

        // 4. Clean up
        component.addOperation("Execute",
            "cmd.exe", "/C",
            "del /Q \"" + archive + "\"",
            "workingdirectory=" + targetDir);

        // 5. Set environment variable
        component.addOperation("Execute",
            "cmd.exe", "/C",
            "setx MNE_DATASETS_SAMPLE_PATH \"" + dataDir + "\"",
            "workingdirectory=" + targetDir);

    } else {
        // --- macOS / Linux ---
        var archive = dataDir + "/MNE-sample-data.tar.gz";

        // 1. Create data directory
        component.addOperation("Mkdir", dataDir);

        // 2. Download with progress (curl shows transfer rate & percentage)
        component.addOperation("Execute",
            "/bin/bash", "-c",
            "echo '=== Downloading MNE Sample Dataset (~1.5 GB) ===' && " +
            "echo 'Source: " + url + "' && " +
            "echo 'Target: " + dataDir + "' && " +
            "echo '' && " +
            "curl -L -# -o '" + archive + "' '" + url + "' 2>&1 | " +
            "  while IFS= read -r -d $'\\r' line; do " +
            "    echo \"  $line\"; " +
            "  done && " +
            "echo '' && echo 'Download complete.'",
            "workingdirectory=" + targetDir,
            "errormessage=Failed to download MNE sample dataset. Check your internet connection.");

        // 3. Extract
        component.addOperation("Execute",
            "/bin/bash", "-c",
            "echo 'Extracting MNE sample dataset...' && " +
            "tar -xzf '" + archive + "' -C '" + dataDir + "' && " +
            "echo 'Extraction complete.'",
            "workingdirectory=" + targetDir,
            "errormessage=Failed to extract MNE sample dataset archive.");

        // 4. Clean up
        component.addOperation("Execute",
            "/bin/bash", "-c",
            "rm -f '" + archive + "'",
            "workingdirectory=" + targetDir);

        // 5. Set environment variable in shell profile
        component.addOperation("Execute",
            "/bin/bash", "-c",
            "SHELL_RC=''; " +
            "[ -f \"$HOME/.zshrc\" ] && SHELL_RC=\"$HOME/.zshrc\"; " +
            "[ -z \"$SHELL_RC\" ] && [ -f \"$HOME/.bashrc\" ] && SHELL_RC=\"$HOME/.bashrc\"; " +
            "[ -z \"$SHELL_RC\" ] && [ -f \"$HOME/.bash_profile\" ] && SHELL_RC=\"$HOME/.bash_profile\"; " +
            "if [ -n \"$SHELL_RC\" ] && ! grep -q MNE_DATASETS_SAMPLE_PATH \"$SHELL_RC\" 2>/dev/null; then " +
            "  echo '' >> \"$SHELL_RC\" && " +
            "  echo '# MNE-CPP: MNE Sample Dataset path' >> \"$SHELL_RC\" && " +
            "  echo 'export MNE_DATASETS_SAMPLE_PATH=\"" + dataDir + "\"' >> \"$SHELL_RC\" && " +
            "  echo 'Environment variable configured.'; " +
            "fi",
            "workingdirectory=" + targetDir);
    }
}

// ---------------------------------------------------------------------------
//  Uninstall: ask whether to remove sample data (it could be large & shared)
// ---------------------------------------------------------------------------
Component.prototype.beginUninstallation = function()
{
    var dataDir = installer.value("TargetDir") + "/data";
    if (systemInfo.kernelType === "winnt") {
        dataDir = installer.value("TargetDir") + "\\data";
    }

    var result = QMessageBox.question(
        "sampledata.remove",
        "Remove Sample Dataset?",
        "The MNE sample dataset is stored at:\n\n" +
        dataDir + "\n\n" +
        "Do you want to delete it? This will free ~1.5 GB of disk space.\n\n" +
        "Choose 'No' to keep the data for use with other tools.",
        QMessageBox.Yes | QMessageBox.No,
        QMessageBox.No
    );

    if (result === QMessageBox.Yes) {
        if (systemInfo.kernelType === "winnt") {
            component.addOperation("Execute",
                "cmd.exe", "/C",
                "rmdir /S /Q \"" + dataDir + "\"",
                "UNDOEXECUTE");
        } else {
            component.addOperation("Execute",
                "/bin/bash", "-c",
                "rm -rf '" + dataDir + "'",
                "UNDOEXECUTE");
        }
    }
}
