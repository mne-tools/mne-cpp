param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('qt', 'ifw')]
    [string]$Kind,

    [Parameter(Mandatory = $true)]
    [string]$Version,

    [ValidateSet('dynamic', 'static')]
    [string]$Linkage,

    [Parameter(Mandatory = $true)]
    [string]$OutputDir,

    [string]$ReleaseTag = $(if ($env:QT_TOOLCHAIN_RELEASE_TAG) { $env:QT_TOOLCHAIN_RELEASE_TAG } else { 'qt_binaries' }),
    [string]$Repository = $(if ($env:GITHUB_REPOSITORY) { $env:GITHUB_REPOSITORY } else { 'mne-tools/mne-cpp' })
)

$ErrorActionPreference = 'Stop'

function Set-PersistentEnvironment {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    if ($env:GITHUB_ENV) {
        Add-Content -Path $env:GITHUB_ENV -Value "${Name}=${Value}"
    }
    Set-Item -Path "Env:${Name}" -Value $Value
}

function Add-PersistentPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathEntry
    )

    if ($env:GITHUB_PATH) {
        Add-Content -Path $env:GITHUB_PATH -Value $PathEntry
    }
    $env:PATH = "${PathEntry};$env:PATH"
}

$versionToken = $Version -replace '\.', ''
switch ($Kind) {
    'qt' {
        if (-not $Linkage) {
            throw '--Linkage is required when -Kind qt'
        }
        $assetName = "qt6_${versionToken}_${Linkage}_binaries_win.zip"
    }
    'ifw' {
        $assetName = "qt_ifw_${versionToken}_win.zip"
    }
}

$downloadDir = Join-Path ([System.IO.Path]::GetTempPath()) ("mnecpp-toolchain-" + [System.Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $downloadDir | Out-Null

try {
    Write-Host "Downloading $assetName from release $ReleaseTag ($Repository)..."
    gh release download $ReleaseTag -R $Repository -p $assetName -D $downloadDir

    if (Test-Path $OutputDir) {
        Remove-Item -Path $OutputDir -Recurse -Force
    }
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
    Expand-Archive -Path (Join-Path $downloadDir $assetName) -DestinationPath $OutputDir -Force

    if ($Kind -eq 'qt') {
        $qtConfigDir = Join-Path $OutputDir 'lib\cmake\Qt6'
        $cmakePrefix = if ($env:CMAKE_PREFIX_PATH) { "$OutputDir;$env:CMAKE_PREFIX_PATH" } else { $OutputDir }

        Set-PersistentEnvironment -Name 'QT_ROOT_DIR' -Value $OutputDir
        Set-PersistentEnvironment -Name 'CMAKE_PREFIX_PATH' -Value $cmakePrefix
        if (Test-Path $qtConfigDir) {
            Set-PersistentEnvironment -Name 'Qt6_DIR' -Value $qtConfigDir
        } else {
            Set-PersistentEnvironment -Name 'Qt6_DIR' -Value $OutputDir
        }
        Add-PersistentPath -PathEntry (Join-Path $OutputDir 'bin')
        Write-Host "Qt toolchain ready at $OutputDir"
    } else {
        Set-PersistentEnvironment -Name 'QtInstallerFramework_DIR' -Value $OutputDir
        Set-PersistentEnvironment -Name 'CPACK_IFW_ROOT' -Value $OutputDir
        Add-PersistentPath -PathEntry (Join-Path $OutputDir 'bin')
        Write-Host "Qt Installer Framework ready at $OutputDir"
    }
}
finally {
    if (Test-Path $downloadDir) {
        Remove-Item -Path $downloadDir -Recurse -Force
    }
}
