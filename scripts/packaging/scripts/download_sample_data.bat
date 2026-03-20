@echo off
REM
REM  download_sample_data.bat
REM  MNE-CPP Installer - Download MNE Sample Dataset
REM
REM  Downloads the MNE sample dataset and sets the MNE_DATASETS_SAMPLE_PATH
REM  environment variable (compatible with MNE-Python conventions).
REM

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "INSTALL_DIR=%SCRIPT_DIR%.."
set "DATA_DIR=%INSTALL_DIR%\data"
set "SAMPLE_DATA_DIR=%DATA_DIR%\MNE-sample-data"
set "DOWNLOAD_URL=https://osf.io/86qa2/download"
set "ARCHIVE_FILE=%DATA_DIR%\MNE-sample-data.tar.gz"

echo ================================================================
echo   MNE-CPP: Download MNE Sample Dataset
echo ================================================================
echo.
echo Download URL: %DOWNLOAD_URL%
echo Destination:  %DATA_DIR%
echo.

REM Create data directory
if not exist "%DATA_DIR%" mkdir "%DATA_DIR%"

REM Check if already downloaded
if exist "%SAMPLE_DATA_DIR%" (
    echo MNE sample dataset already exists at: %SAMPLE_DATA_DIR%
    echo To re-download, remove the directory and run this script again.
    goto :setenv
)

echo Downloading MNE sample dataset (~1.5 GB^)...
echo This may take a while depending on your internet connection.
echo.

REM Use PowerShell to download (available on Windows 10+)
powershell -Command "& { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; $ProgressPreference = 'Continue'; Invoke-WebRequest -Uri '%DOWNLOAD_URL%' -OutFile '%ARCHIVE_FILE%' -UseBasicParsing }"

if %errorlevel% neq 0 (
    echo ERROR: Download failed. Please check your internet connection.
    exit /b 1
)

echo.
echo Extracting archive...

REM Use tar (built into Windows 10+) or PowerShell
where tar >nul 2>&1
if %errorlevel% equ 0 (
    tar -xzf "%ARCHIVE_FILE%" -C "%DATA_DIR%"
) else (
    powershell -Command "& { Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('%ARCHIVE_FILE%', '%DATA_DIR%') }"
)

if exist "%ARCHIVE_FILE%" del "%ARCHIVE_FILE%"

echo Download and extraction complete.

:setenv
echo.
echo Sample data location: %SAMPLE_DATA_DIR%
echo.
echo ================================================================
echo   Environment Variable Setup
echo ================================================================
echo.

REM Set user environment variable
echo Setting MNE_DATASETS_SAMPLE_PATH environment variable...
setx MNE_DATASETS_SAMPLE_PATH "%DATA_DIR%" >nul 2>&1

if %errorlevel% equ 0 (
    echo MNE_DATASETS_SAMPLE_PATH has been set to: %DATA_DIR%
    echo This change will take effect in new command prompts.
) else (
    echo WARNING: Could not set environment variable automatically.
    echo Please set it manually:
    echo   setx MNE_DATASETS_SAMPLE_PATH "%DATA_DIR%"
)

echo.
echo ================================================================
echo   Done!
echo ================================================================

endlocal
pause
