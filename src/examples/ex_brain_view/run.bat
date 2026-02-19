@echo off
setlocal enabledelayedexpansion

:: Default values
if defined MNE_DATA_PATH (
    set "SubjectPath=%MNE_DATA_PATH%\subjects"
    set "MneSampleDataPath=%MNE_DATA_PATH%"
) else (
    set "SubjectPath=%USERPROFILE%\mne_data\MNE-sample-data\subjects"
    set "MneSampleDataPath=%USERPROFILE%\mne_data\MNE-sample-data"
)
set "Subject=sample"
set "Hemi=0"

:: Default data paths
set "SampleDir=%MneSampleDataPath%\MEG\sample"
set "ProcessedDir=%SampleDir%\processed"
set "DigitizerFile=%SampleDir%\sample_audvis_raw.fif"
set "TransFile=%SampleDir%\all-trans.fif"
set "EvokedFile=%SampleDir%\sample_audvis-ave.fif"

:: Default build path relative to script location
set "ScriptDir=%~dp0"
set "BuildPath=%ScriptDir%..\..\..\out\Release\examples\ex_brain_view.exe"

:: Kill any existing instances
taskkill /f /im ex_brain_view.exe >nul 2>&1

:: Check if executable exists
if not exist "%BuildPath%" (
    echo Error: Could not locate ex_brain_view executable at %BuildPath%
    exit /b 1
)

set "BemFile=%SubjectPath%\%Subject%\bem\sample-5120-5120-5120-bem.fif"
set "SrcSpaceFile=%SubjectPath%\%Subject%\bem\sample-oct-6-orig-src.fif"
set "AtlasFile=%SubjectPath%\%Subject%\label\lh.aparc.annot"

echo launching ex_brain_view from %BuildPath%...

:: Build --stc arguments: load all *-lh.stc files from the processed\ directory
set "STC_ARGS="
if exist "%ProcessedDir%" (
    for %%F in ("%ProcessedDir%\*-lh.stc") do (
        set "STC_ARGS=!STC_ARGS! --stc "%%F""
    )
)
:: Fallback to the original sample STC if no processed STCs exist
if "%STC_ARGS%"=="" (
    set "FallbackStc=%SampleDir%\sample_audvis-meg-eeg-lh.stc"
    if exist "!FallbackStc!" set "STC_ARGS=--stc "!FallbackStc!""
)

"%BuildPath%" --subjectPath "%SubjectPath%" --subject "%Subject%" --hemi "%Hemi%" --bem "%BemFile%" ^
    %STC_ARGS% --digitizer "%DigitizerFile%" --trans "%TransFile%" --srcSpace "%SrcSpaceFile%" ^
    --atlas "%AtlasFile%" --evoked "%EvokedFile%"

endlocal
