@echo off
setlocal enabledelayedexpansion

:: Default data paths
if defined MNE_DATA_PATH (
    set "MneSampleDataPath=%MNE_DATA_PATH%"
) else (
    set "MneSampleDataPath=%USERPROFILE%\mne_data\MNE-sample-data"
)
set "SampleDir=%MneSampleDataPath%\MEG\sample"
set "RawFile=%SampleDir%\sample_audvis_raw.fif"
set "EventsFile=%SampleDir%\sample_audvis-eve.fif"

:: Default build path relative to script location
set "ScriptDir=%~dp0"
set "BuildPath=%ScriptDir%..\..\..\out\Release\bin\mne_browse.exe"

:: Fall back to Coverage then Debug if Release binary is missing
if not exist "%BuildPath%" (
    set "BuildPath=%ScriptDir%..\..\..\out\Coverage\bin\mne_browse.exe"
)
if not exist "%BuildPath%" (
    set "BuildPath=%ScriptDir%..\..\..\out\Debug\bin\mne_browse.exe"
)

:: Kill any existing instances
taskkill /f /im mne_browse.exe >nul 2>&1

:: Verify executable
if not exist "%BuildPath%" (
    echo Error: Could not locate mne_browse executable.
    echo Checked Release, Coverage and Debug under out\
    echo Build the application first:
    echo   cmake --build build --config Release --target mne_browse
    exit /b 1
)

:: Verify sample data
if not exist "%RawFile%" (
    echo Error: Sample data file not found: %RawFile%
    echo Download MNE sample data to %MneSampleDataPath% or set MNE_DATA_PATH.
    exit /b 1
)

echo Launching MNE Browse from %BuildPath%...

:: Load events file if it exists
set "EXTRA_ARGS="
if exist "%EventsFile%" set "EXTRA_ARGS=--events "%EventsFile%""

"%BuildPath%" --raw "%RawFile%" %EXTRA_ARGS%

endlocal
