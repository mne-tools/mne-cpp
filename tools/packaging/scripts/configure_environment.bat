@echo off
REM
REM  configure_environment.bat
REM  MNE-CPP Installer - PATH and Environment Variable Configuration
REM
REM  Configures system PATH and environment variables for MNE-CPP:
REM   - Adds bin\ to PATH (for applications)
REM   - Sets MNE_CPP_ROOT pointing to the installation directory
REM   - Sets MNE_CPP_SDK (include + lib paths for development)
REM   - Sets MNE_DATASETS_SAMPLE_PATH if sample data is installed
REM

setlocal enabledelayedexpansion

set "SCRIPT_DIR=%~dp0"
set "INSTALL_DIR=%SCRIPT_DIR%.."
set "BIN_DIR=%INSTALL_DIR%\bin"
set "LIB_DIR=%INSTALL_DIR%\lib"
set "INCLUDE_DIR=%INSTALL_DIR%\include"
set "DATA_DIR=%INSTALL_DIR%\data"

echo ================================================================
echo   MNE-CPP: Environment Configuration
echo ================================================================
echo.
echo Installation directory: %INSTALL_DIR%
echo.

REM --- MNE_CPP_ROOT ---
echo Setting MNE_CPP_ROOT...
setx MNE_CPP_ROOT "%INSTALL_DIR%" >nul 2>&1
if %errorlevel% equ 0 (
    echo   [set] MNE_CPP_ROOT = %INSTALL_DIR%
) else (
    echo   [WARNING] Could not set MNE_CPP_ROOT automatically.
)

REM --- PATH: Add binaries ---
if exist "%BIN_DIR%" (
    echo Adding bin\ to PATH...

    REM Read current user PATH
    for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v PATH 2^>nul') do set "CURRENT_PATH=%%b"

    REM Check if already in PATH
    echo !CURRENT_PATH! | findstr /I /C:"%BIN_DIR%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [skip] bin\ already on PATH
    ) else (
        setx PATH "%BIN_DIR%;!CURRENT_PATH!" >nul 2>&1
        if !errorlevel! equ 0 (
            echo   [added] %BIN_DIR% to PATH
        ) else (
            echo   [WARNING] Could not add to PATH automatically.
            echo   Please add manually: %BIN_DIR%
        )
    )

    REM Also add lib\ to PATH (Windows uses PATH for DLL lookup)
    echo Adding lib\ to PATH for DLL lookup...
    echo !CURRENT_PATH! | findstr /I /C:"%LIB_DIR%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [skip] lib\ already on PATH
    ) else (
        REM Re-read PATH since we may have just modified it
        for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v PATH 2^>nul') do set "CURRENT_PATH=%%b"
        setx PATH "%LIB_DIR%;!CURRENT_PATH!" >nul 2>&1
        if !errorlevel! equ 0 (
            echo   [added] %LIB_DIR% to PATH
        ) else (
            echo   [WARNING] Could not add lib\ to PATH.
        )
    )
) else (
    echo   [skip] bin\ directory not found ^(applications not installed^)
)

REM --- Development SDK ---
if exist "%INCLUDE_DIR%" (
    echo Setting MNE_CPP_SDK...
    setx MNE_CPP_SDK "%INSTALL_DIR%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [set] MNE_CPP_SDK = %INSTALL_DIR%
    ) else (
        echo   [WARNING] Could not set MNE_CPP_SDK.
    )

    REM Add to CMAKE_PREFIX_PATH
    echo Setting CMAKE_PREFIX_PATH hint for SDK...
    for /f "tokens=2*" %%a in ('reg query "HKCU\Environment" /v CMAKE_PREFIX_PATH 2^>nul') do set "CURRENT_CMAKE_PATH=%%b"
    if defined CURRENT_CMAKE_PATH (
        echo !CURRENT_CMAKE_PATH! | findstr /I /C:"%INSTALL_DIR%" >nul 2>&1
        if !errorlevel! equ 0 (
            echo   [skip] MNE-CPP already in CMAKE_PREFIX_PATH
        ) else (
            setx CMAKE_PREFIX_PATH "%INSTALL_DIR%;!CURRENT_CMAKE_PATH!" >nul 2>&1
            echo   [added] %INSTALL_DIR% to CMAKE_PREFIX_PATH
        )
    ) else (
        setx CMAKE_PREFIX_PATH "%INSTALL_DIR%" >nul 2>&1
        echo   [set] CMAKE_PREFIX_PATH = %INSTALL_DIR%
    )

    echo.
    echo   SDK Usage Hints:
    echo     CMake:    find_package(MNE-CPP REQUIRED)
    echo     Include:  /I%%MNE_CPP_SDK%%\include
    echo     Link:     /LIBPATH:%%MNE_CPP_SDK%%\lib mne_utils.lib mne_fiff.lib ...
) else (
    echo   [skip] include\ not found ^(Development SDK not installed^)
)

REM --- MNE Sample Dataset ---
if exist "%DATA_DIR%\MNE-sample-data" (
    echo Setting MNE_DATASETS_SAMPLE_PATH...
    setx MNE_DATASETS_SAMPLE_PATH "%DATA_DIR%" >nul 2>&1
    if !errorlevel! equ 0 (
        echo   [set] MNE_DATASETS_SAMPLE_PATH = %DATA_DIR%
    ) else (
        echo   [WARNING] Could not set MNE_DATASETS_SAMPLE_PATH.
    )
)

echo.
echo ================================================================
echo   Configuration complete!
echo ================================================================
echo.
echo Environment variables have been set for your user account.
echo Changes will take effect in new command prompts.
echo.
echo To apply immediately in this session, close and reopen your terminal.
echo ================================================================

endlocal
pause
