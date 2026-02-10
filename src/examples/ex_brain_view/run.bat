@echo off
setlocal

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
set "StcFile=%MneSampleDataPath%\MEG\sample\sample_audvis-meg-eeg-lh.stc"
set "DigitizerFile=%MneSampleDataPath%\MEG\sample\sample_audvis_raw.fif"
set "TransFile=%MneSampleDataPath%\MEG\sample\all-trans.fif"

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
"%BuildPath%" --subjectPath "%SubjectPath%" --subject "%Subject%" --hemi "%Hemi%" --bem "%BemFile%" ^
    --stc "%StcFile%" --digitizer "%DigitizerFile%" --trans "%TransFile%" --srcSpace "%SrcSpaceFile%" ^
    --atlas "%AtlasFile%"

endlocal
