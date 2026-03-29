param(
    [Parameter(Mandatory = $true)]
    [string]$Url,

    [Parameter(Mandatory = $true)]
    [string]$TargetDir,

    [Parameter(Mandatory = $true)]
    [string]$AssetName
)

$ErrorActionPreference = "Stop"

function Get-PayloadRoot {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    if ((Test-Path (Join-Path $Root "bin") -PathType Container) -or
        (Test-Path (Join-Path $Root "apps") -PathType Container)) {
        return $Root
    }

    $candidate = Get-ChildItem -Path $Root -Directory -Recurse |
        Where-Object { $_.Name -eq "bin" -or $_.Name -eq "apps" } |
        Select-Object -First 1

    if ($candidate) {
        return Split-Path -Path $candidate.FullName -Parent
    }

    return $Root
}

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("mne-cpp-installer-" + [Guid]::NewGuid().ToString("N"))
$archivePath = Join-Path $tempRoot $AssetName
$extractDir = Join-Path $tempRoot "extract"

New-Item -ItemType Directory -Force -Path $extractDir | Out-Null

try {
    Write-Host "================================================================"
    Write-Host "  MNE-CPP: Downloading selected release archive"
    Write-Host "================================================================"
    Write-Host "URL       : $Url"
    Write-Host "Asset     : $AssetName"
    Write-Host "Target    : $TargetDir"
    Write-Host ""

    $ProgressPreference = "Continue"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $Url -OutFile $archivePath -UseBasicParsing

    Write-Host ""
    Write-Host "Extracting release archive..."

    if ($AssetName.ToLowerInvariant().EndsWith(".zip")) {
        Expand-Archive -Path $archivePath -DestinationPath $extractDir -Force
    } elseif ($AssetName.ToLowerInvariant().EndsWith(".tar.gz")) {
        tar -xzf $archivePath -C $extractDir
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to extract $AssetName"
        }
    } else {
        throw "Unsupported archive type: $AssetName"
    }

    $payloadRoot = Get-PayloadRoot -Root $extractDir
    Write-Host "Payload root: $payloadRoot"

    New-Item -ItemType Directory -Force -Path $TargetDir | Out-Null
    Get-ChildItem -Path $payloadRoot -Force | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $TargetDir -Recurse -Force
    }

    Write-Host ""
    Write-Host "MNE-CPP release archive installed successfully."
} finally {
    if (Test-Path $tempRoot) {
        Remove-Item -Path $tempRoot -Recurse -Force -ErrorAction SilentlyContinue
    }
}
