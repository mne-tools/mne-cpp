@echo off
REM
REM  install_mne_python.bat
REM  MNE-CPP Installer - Install MNE-Python into a Virtual Environment
REM
REM  Creates a Python virtual environment inside the MNE-CPP installation
REM  directory and installs MNE-Python into it.  This avoids conflicts with
REM  system-managed Python packages (PEP 668) and keeps the installation
REM  self-contained.
REM
REM  Usage:
REM    install_mne_python.bat [INSTALL_DIR]
REM

setlocal enabledelayedexpansion

echo ================================================================
echo   MNE-CPP: Install MNE-Python (Virtual Environment)
echo ================================================================
echo.

REM ---------------------------------------------------------------------------
REM Determine installation directory
REM ---------------------------------------------------------------------------
if "%~1"=="" (
    set "SCRIPT_DIR=%~dp0"
    for %%I in ("!SCRIPT_DIR!\..") do set "INSTALL_DIR=%%~fI"
) else (
    set "INSTALL_DIR=%~f1"
)

set "VENV_DIR=!INSTALL_DIR!\mne-python-env"

echo Installation directory : !INSTALL_DIR!
echo Virtual environment    : !VENV_DIR!
echo.

REM ---------------------------------------------------------------------------
REM Locate Python 3
REM ---------------------------------------------------------------------------
set "PYTHON_CMD="

where python3 >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%v in ('python3 -c "import sys; print(sys.version_info.major)" 2^>nul') do (
        if "%%v"=="3" (
            set "PYTHON_CMD=python3"
            goto :found_python
        )
    )
)

where python >nul 2>&1
if !errorlevel! equ 0 (
    for /f "tokens=*" %%v in ('python -c "import sys; print(sys.version_info.major)" 2^>nul') do (
        if "%%v"=="3" (
            set "PYTHON_CMD=python"
            goto :found_python
        )
    )
)

echo ERROR: Python 3 not found on the system PATH.
echo.
echo Please install Python 3 from https://www.python.org/downloads/
echo and ensure it is added to your PATH, then run this script again.
exit /b 1

:found_python
echo Found Python:
!PYTHON_CMD! --version
echo.

REM ---------------------------------------------------------------------------
REM Verify the venv module is available
REM ---------------------------------------------------------------------------
!PYTHON_CMD! -m venv --help >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: The Python 'venv' module is not available.
    echo.
    echo Ensure you have a standard Python 3 installation from python.org
    echo with the "pip" and "venv" options enabled.
    exit /b 1
)

REM ---------------------------------------------------------------------------
REM Create virtual environment (or reuse existing)
REM ---------------------------------------------------------------------------
if exist "!VENV_DIR!\Scripts\python.exe" (
    echo Virtual environment already exists at: !VENV_DIR!
    echo Reusing existing environment.
) else (
    echo Creating virtual environment...
    !PYTHON_CMD! -m venv "!VENV_DIR!"
    if !errorlevel! neq 0 (
        echo ERROR: Failed to create virtual environment.
        exit /b 1
    )
    echo Virtual environment created.
)
echo.

REM ---------------------------------------------------------------------------
REM Install MNE-Python inside the venv
REM ---------------------------------------------------------------------------
set "VENV_PIP=!VENV_DIR!\Scripts\pip.exe"
set "VENV_PYTHON=!VENV_DIR!\Scripts\python.exe"

if not exist "!VENV_PIP!" (
    echo ERROR: pip not found in virtual environment at !VENV_PIP!
    echo The virtual environment may be corrupt. Try deleting !VENV_DIR! and running again.
    exit /b 1
)

echo Upgrading pip...
"!VENV_PYTHON!" -m pip install --upgrade pip >nul 2>&1
echo.

echo Installing MNE-Python...
echo.
"!VENV_PIP!" install --upgrade mne

if !errorlevel! equ 0 (
    echo.
    echo ================================================================
    echo   MNE-Python installed successfully!
    echo ================================================================
    echo.
    echo Virtual environment: !VENV_DIR!
    echo.
    echo To use MNE-Python, activate the environment first:
    echo   "!VENV_DIR!\Scripts\activate.bat"
    echo.
    echo Then verify with:
    echo   python -c "import mne; print(mne.__version__)"
    echo.
    echo To get started, visit: https://mne.tools/stable/auto_tutorials/index.html
) else (
    echo.
    echo ERROR: MNE-Python installation failed.
    echo Please try manually:
    echo   "!VENV_PIP!" install mne
    exit /b 1
)

REM ---------------------------------------------------------------------------
REM Create a convenience wrapper script
REM ---------------------------------------------------------------------------
set "WRAPPER=!INSTALL_DIR!\bin\mne-python.bat"
if not exist "!INSTALL_DIR!\bin" mkdir "!INSTALL_DIR!\bin"
(
    echo @echo off
    echo REM MNE-Python convenience wrapper
    echo set "VENV_DIR=%%~dp0..\mne-python-env"
    echo if not exist "%%VENV_DIR%%\Scripts\python.exe" ^(
    echo     echo ERROR: MNE-Python virtual environment not found.
    echo     echo Please run install_mne_python.bat first.
    echo     exit /b 1
    echo ^)
    echo "%%VENV_DIR%%\Scripts\python.exe" %%*
) > "!WRAPPER!"
echo Wrapper script created: !WRAPPER!
echo You can run: mne-python -c "import mne; print(mne.__version__)"

endlocal
