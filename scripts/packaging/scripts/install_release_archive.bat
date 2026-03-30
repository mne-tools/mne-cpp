@echo off
setlocal EnableDelayedExpansion

set "URL="
set "TARGET_DIR="
set "ASSET_NAME="

:parse_args
if "%~1"=="" goto after_parse
if /I "%~1"=="--url" (
    set "URL=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--target-dir" (
    set "TARGET_DIR=%~2"
    shift
    shift
    goto parse_args
)
if /I "%~1"=="--asset-name" (
    set "ASSET_NAME=%~2"
    shift
    shift
    goto parse_args
)
shift
goto parse_args

:after_parse
if not defined URL goto usage_fail
if not defined TARGET_DIR goto usage_fail
if not defined ASSET_NAME goto usage_fail

set "TMP_DIR=%TEMP%\mne-cpp-installer-%RANDOM%%RANDOM%%RANDOM%"
set "ARCHIVE_PATH=%TMP_DIR%\%ASSET_NAME%"
set "EXTRACT_DIR=%TMP_DIR%\extract"

if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"
mkdir "%EXTRACT_DIR%" || goto fail

echo ================================================================
echo   MNE-CPP: Downloading selected release archive
echo ================================================================
echo URL       : %URL%
echo Asset     : %ASSET_NAME%
echo Target    : %TARGET_DIR%
echo.

curl --fail --location --silent --show-error --output "%ARCHIVE_PATH%" "%URL%"
if errorlevel 1 goto fail

echo.
echo Extracting release archive...

tar -xf "%ARCHIVE_PATH%" -C "%EXTRACT_DIR%"
if errorlevel 1 goto fail

call :find_payload_root "%EXTRACT_DIR%"
if errorlevel 1 goto fail

echo Payload root: %PAYLOAD_ROOT%
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
xcopy "%PAYLOAD_ROOT%\*" "%TARGET_DIR%\" /E /I /Y /Q >nul
if errorlevel 1 goto fail

echo.
echo MNE-CPP release archive installed successfully.
if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"
exit /b 0

:find_payload_root
set "PAYLOAD_ROOT=%~1"
if exist "%~1\bin" exit /b 0
if exist "%~1\apps" exit /b 0

for /d /r "%~1" %%D in (*) do (
    if /I "%%~nxD"=="bin" (
        for %%P in ("%%~dpD..") do set "PAYLOAD_ROOT=%%~fP"
        exit /b 0
    )
    if /I "%%~nxD"=="apps" (
        for %%P in ("%%~dpD..") do set "PAYLOAD_ROOT=%%~fP"
        exit /b 0
    )
)
exit /b 0

:usage_fail
echo ERROR: Missing required arguments. 1>&2
echo Usage: install_release_archive.bat --url ^<url^> --target-dir ^<dir^> --asset-name ^<file^> 1>&2
exit /b 1

:fail
if exist "%TMP_DIR%" rmdir /s /q "%TMP_DIR%"
exit /b 1
