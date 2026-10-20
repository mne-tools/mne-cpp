# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Add the MNE-CPP bin folder to the user PATH and set MNE_CPP_ROOT, or remove both.
# Installed into <install dir>\scripts by the installer.
#
# Usage: configure_environment.ps1 install|uninstall

param([Parameter(Mandatory = $true)][ValidateSet('install', 'uninstall')][string]$Action)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$bin = Join-Path $root 'bin'

# Edit the unexpanded registry value so entries such as %USERPROFILE%\... survive
$key = Get-Item 'HKCU:\Environment'
$entries = @($key.GetValue('Path', '', 'DoNotExpandEnvironmentNames') -split ';' |
    Where-Object { $_ -and ($_.TrimEnd('\') -ne $bin.TrimEnd('\')) })
if ($Action -eq 'install') {
    $entries = @($bin) + $entries
}
Set-ItemProperty -Path 'HKCU:\Environment' -Name 'Path' -Value ($entries -join ';') -Type ExpandString

# SetEnvironmentVariable also tells running programs such as Explorer that the environment changed
if ($Action -eq 'install') {
    [Environment]::SetEnvironmentVariable('MNE_CPP_ROOT', $root, 'User')
    Write-Output "MNE-CPP: added $bin to the user PATH"
} else {
    [Environment]::SetEnvironmentVariable('MNE_CPP_ROOT', $null, 'User')
    Write-Output 'MNE-CPP: removed the PATH entry'
}
