@echo off
REM
REM  install_mne_python.bat
REM  MNE-CPP Installer - Install MNE-Python via pip
REM
REM  Installs the MNE-Python package using pip.
REM  Requires Python 3 and pip to be available on the system.
REM

setlocal enabledelayedexpansion

echo ================================================================
echo   MNE-CPP: Install MNE-Python
echo ================================================================
echo.

REM Find Python 3
set "PYTHON_CMD="

where python3 >nul 2>&1
if %errorlevel% equ 0 (
    set "PYTHON_CMD=python3"
    goto :found_python
)

where python >nul 2>&1
if %errorlevel% equ 0 (
    REM Check if it's Python 3
    for /f "tokens=2 delims= " %%v in ('python --version 2^>^&1') do (
        set "PY_VER=%%v"
    )
    if "!PY_VER:~0,1!"=="3" (
        set "PYTHON_CMD=python"
        goto :found_python
    )
)

echo ERROR: Python 3 not found on the system PATH.
echo.
echo Please install Python 3 from https://www.python.org/downloads/
echo and ensure it is added to your PATH, then run this script again.
exit /b 1

:found_python
echo Found Python:
%PYTHON_CMD% --version
echo.

REM Check for pip (try python -m pip first, then pip3, then pip)
set "PIP_CMD="

%PYTHON_CMD% -m pip --version >nul 2>&1
if %errorlevel% equ 0 (
    set "PIP_CMD=%PYTHON_CMD% -m pip"
    goto :found_pip
)

where pip3 >nul 2>&1
if %errorlevel% equ 0 (
    set "PIP_CMD=pip3"
    goto :found_pip
)

where pip >nul 2>&1
if %errorlevel% equ 0 (
    set "PIP_CMD=pip"
    goto :found_pip
)

echo ERROR: pip is not available.
echo.
echo Neither 'python -m pip', 'pip3', nor 'pip' were found.
echo Please install pip:
echo   %PYTHON_CMD% -m ensurepip --upgrade
exit /b 1

:found_pip
echo Found pip:
!PIP_CMD! --version
echo.

REM Install MNE-Python
echo Installing MNE-Python...
echo.
!PIP_CMD! install --upgrade mne

if %errorlevel% equ 0 (
    echo.
    echo ================================================================
    echo   MNE-Python installed successfully!
    echo ================================================================
    echo.
    echo You can verify the installation with:
    echo   %PYTHON_CMD% -c "import mne; print(mne.__version__)"
    echo.
    echo To get started, visit: https://mne.tools/stable/auto_tutorials/index.html
) else (
    echo.
    echo ERROR: MNE-Python installation failed.
    echo Please try running manually: !PIP_CMD! install mne
    exit /b 1
)

endlocal
pause
