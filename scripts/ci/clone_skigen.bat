@echo off
rem SPDX-License-Identifier: BSD-3-Clause
rem Copyright (c) 2010-2026 MNE-CPP Authors
rem
rem Clone Skigen at the centrally-configured git ref.
rem
rem The ref is resolved from src\external\external_deps.env:
rem   --release  -> SKIGEN_RELEASE_REF (immutable tag, reproducible builds)
rem   (default)  -> SKIGEN_DEV_REF     (moving development branch)
rem An explicit --ref <ref> overrides both.

setlocal EnableDelayedExpansion

set "SCRIPT_DIR=%~dp0"
set "SKIGEN_RELEASE_REF="
set "SKIGEN_DEV_REF="

rem ── Load central ref config (KEY=VALUE lines, ignore comments) ──
for /f "usebackq tokens=1,2 delims==" %%A in ("%SCRIPT_DIR%..\..\src\external\external_deps.env") do (
    set "key=%%A"
    set "key=!key: =!"
    if not "!key!"=="" if not "!key:~0,1!"=="#" set "%%A=%%B"
)

set "USE_RELEASE=0"
set "EXPLICIT_REF="
set "OUTPUT_DIR=src\external\skigen"
set "DEPTH=1"

:parse
if "%~1"=="" goto resolve
if /i "%~1"=="--release" ( set "USE_RELEASE=1" & shift & goto parse )
if /i "%~1"=="--ref" ( set "EXPLICIT_REF=%~2" & shift & shift & goto parse )
if /i "%~1"=="--output-dir" ( set "OUTPUT_DIR=%~2" & shift & shift & goto parse )
if /i "%~1"=="--depth" ( set "DEPTH=%~2" & shift & shift & goto parse )
echo Unknown argument: %~1
exit /b 1

:resolve
if defined EXPLICIT_REF (
    set "REF=%EXPLICIT_REF%"
) else if "%USE_RELEASE%"=="1" (
    set "REF=%SKIGEN_RELEASE_REF%"
) else (
    set "REF=%SKIGEN_DEV_REF%"
)

set "DEPTH_ARG=--depth %DEPTH%"
if "%DEPTH%"=="0" set "DEPTH_ARG="

echo Cloning skigen (%REF%) into %OUTPUT_DIR%...
git clone %DEPTH_ARG% --branch %REF% https://github.com/skigen-project/skigen.git "%OUTPUT_DIR%"
exit /b %ERRORLEVEL%
