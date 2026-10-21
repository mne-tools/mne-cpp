@echo off
REM SPDX-License-Identifier: BSD-3-Clause
REM Copyright (c) 2010-2026 MNE-CPP Authors
REM   Christoph Dinh <christoph.dinh@mne-cpp.org>
REM
REM Collects the Qt and library dependencies of a Windows build and, with
REM "pack", archives them for release. macOS and Linux use deploy.sh.

REM setlocal EnableDelayedExpansion

SET SCRIPT_PATH=%~dp0
SET BASE_PATH=%SCRIPT_PATH%..\..
SET CURRENT_PATH=%cd%

REM SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\"
SETX VCINSTALLDIR "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\"

SET "LINK_OPTION=dynamic"
SET "PACK_OPTION=false"
SET "BUILD_NAME=Release"
SET "MOCK_BUILD=false"
SET "MOCK_TEXT="

:loop
IF NOT "%1"=="" (
    IF "%1"=="dynamic" (
        SET "LINK_OPTION=dynamic"
    )
    IF "%1"=="static" (
        SET "LINK_OPTION=static"
    )   
    IF "%1"=="pack" (
        SET "PACK_OPTION=true"
    )   
    IF "%1"=="help" (
        GOTO :showHelp
        GOTO :endOfScript
    )   
    IF "%1"=="mock" (
        SET "MOCK_BUILD=true"
    )
    IF "%1"=="build-name" (
        IF NOT "%2"=="" (
            SET "BUILD_NAME=%2"
            SHIFT
        )
    )
    SHIFT
    GOTO :loop
)

SET "OUT_DIR_NAME=%BASE_PATH%\out\%BUILD_NAME%"

IF "%MOCK_BUILD%"=="true" (
    ECHO.
    ECHO Mock mode ON. Commands to be executed: 
    ECHO.
    SET "MOCK_TEXT=ECHO "
)

call :doPrintConfiguration

IF "%LINK_OPTION%"=="dynamic" (

    ECHO Checking for windeployqt...
    where windeployqt
    IF ERRORLEVEL 1 (
        ECHO ERROR: windeployqt not found on PATH. Ensure Qt bin\ is on PATH.
        exit /B 1
    )

    ECHO Changing to output directory: %OUT_DIR_NAME%\bin
    IF NOT EXIST "%OUT_DIR_NAME%\bin" (
        ECHO ERROR: Output directory does not exist: %OUT_DIR_NAME%\bin
        ECHO The build may have failed or used a different output path.
        exit /B 1
    )
    cd /d %OUT_DIR_NAME%\bin

    ECHO.
    ECHO Files in bin directory before windeployqt:
    dir /b *.exe 2>nul
    dir /b *.dll 2>nul
    ECHO.

    REM Solve dependencies for top-level DLLs only (not plugin subdirs).
    REM Using /b without /s to avoid recursing into mne_scan_plugins/ etc.
    REM which would bloat each plugin dir with duplicate Qt DLLs.
    SET "DLL_COUNT=0"
    for /f %%f in ('dir /b *.dll 2^>nul') do (
      ECHO Running windeployqt on %%f ...
      %MOCK_TEXT%windeployqt %%f
      SET /A DLL_COUNT+=1
    )

    REM Solve dependencies for top-level applications only
    SET "EXE_COUNT=0"
    for /f %%f in ('dir /b *.exe 2^>nul') do (
        ECHO Running windeployqt on %%f ...
        %MOCK_TEXT%windeployqt %%f
        SET /A EXE_COUNT+=1
    )

    ECHO.
    ECHO windeployqt summary: processed DLLs and EXEs
    ECHO.

    REM REM solve dependencies for tests 
    REM for /f %%f in ('dir test_*.exe /s /b') do (
    REM     %MOCK_TEXT%windeployqt %%f
    REM )
    REM
    REM REM solve dependencies for examples 
    REM for /f %%f in ('dir ex_*.exe /s /b') do (
    REM     %MOCK_TEXT%windeployqt %%f
    REM )

    IF "%PACK_OPTION%"=="true" (
        cd /d %BASE_PATH%
        REM Delete folders which we do not want to ship
        %MOCK_TEXT%rmdir %OUT_DIR_NAME%\resources\data /s /q

        REM Install SDK components (headers + CMake config) into the output tree
        ECHO.
        ECHO === Installing SDK components (headers + CMake config) ===
        %MOCK_TEXT%cmake --install %BASE_PATH%\build\%BUILD_NAME% --prefix %OUT_DIR_NAME% --component sdk
        REM Install Eigen headers and CMake config (used by MNE-CPP public headers)
        %MOCK_TEXT%cmake --install %BASE_PATH%\build\%BUILD_NAME% --prefix %OUT_DIR_NAME% --component Devel

        REM Creating archive from inside output directory for clean top-level layout
        cd /d %OUT_DIR_NAME%
        ECHO.
        ECHO Contents of bin directory:
        dir bin\*.dll 2>nul | find "File(s)"
        dir bin\*.exe 2>nul | find "File(s)"
        ECHO Checking for Qt DLLs...
        dir bin\Qt6*.dll 2>nul | find "File(s)"
        ECHO Checking for platform plugins...
        IF EXIST bin\platforms (
            ECHO   platforms\ directory found
        ) ELSE (
            ECHO   WARNING: platforms\ directory NOT found - windeployqt may have failed
        )
        ECHO.
        REM Only include lib if it exists (on Windows, DLLs are in bin/)
        SET "PACK_DIRS=bin include share resources"
        IF EXIST lib (
            SET "PACK_DIRS=bin lib include share resources"
        )
        %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-dynamic-x86_64.zip %PACK_DIRS%
        cd /d %BASE_PATH%
    )

    cd %cd%

) ELSE IF "%LINK_OPTION%"=="static" (

    IF "%PACK_OPTION%"=="true" (
        REM This script needs to be run from the top level mne-cpp repo folder
        REM Delete folders which we do not want to ship
        %MOCK_TEXT%rmdir %OUT_DIR_NAME%\resources\data /s /q
        %MOCK_TEXT%rmdir %OUT_DIR_NAME%\bin\mne_rt_server_plugins /s /q
        %MOCK_TEXT%rmdir %OUT_DIR_NAME%\bin\mne_scan_plugins /s /q
        %MOCK_TEXT%rmdir %OUT_DIR_NAME%\bin\mne_analyze_plugins /s /q

        REM Creating archive of everything in the bin directory
        %MOCK_TEXT%7z a %BASE_PATH%\mne-cpp-windows-static-x86_64.zip %OUT_DIR_NAME%
    )

) ELSE (
    ECHO Your link option: %LINK_OPTION% is not defined.
    goto :showHelp
    goto :endOfScript
)

:endOfScript

exit /B

:showHelp
  ECHO. 
  ECHO MNE-CPP Deplyment script help.
  ECHO. 
  ECHO Usage: ./deploy.bat [Options]
  ECHO.
  ECHO [help]  - Print this help.
  ECHO [dynamic/static] - Set the link type as dynamic (default) or static.
  ECHO [pack] - Enable output packaging into a compressed file.
  ECHO [build-name=]  - Specify the name of the build to deploy and 
  ECHO                         (if specified) pack.
  ECHO.
exit /B 0

:doPrintConfiguration
  ECHO.
  ECHO ====================================================================
  ECHO ===================== MNE-CPP DEPLOY SCRIPT =========================
  ECHO.
  ECHO LINK_OPTION  = %LINK_OPTION%
  ECHO PACK_OPTION  = %PACK_OPTION%
  ECHO BUILD_NAME   = %BUILD_NAME%
  ECHO OUT_DIR_NAME = %OUT_DIR_NAME%
  ECHO MOCK_BUILD   = %MOCK_BUILD%
  ECHO MOCK_TEXT    = %MOCK_TEXT%
  ECHO.
  ECHO ====================================================================
  ECHO ====================================================================
  ECHO.
exit /B 0

cd %CD%

exit /b
