@echo off
setlocal
::
:: run.bat - Run mne_compute_raw_inverse with MNE sample data
::
:: Computes the full source space dSPM inverse solution of
:: sample_audvis-ave.fif using the MEG+EEG inverse operator
:: for all evoked data sets (Left Auditory, Right Auditory,
:: Left visual, Right visual).
::
:: Output is written to <sample-data>\MEG\sample\processed\
::
:: Parameters:
::   SNR     = 3      (lambda2 = 1/9)
::   method  = dSPM   (--spm)
::   sets    = all    (default, no --set)
::   baseline: -200 to 0 ms  (matching mne-python baseline=(None, 0))
::

:: Default MNE sample data path
if defined MNE_DATA_PATH (
    set "MneSampleDataPath=%MNE_DATA_PATH%"
) else (
    set "MneSampleDataPath=%USERPROFILE%\mne_data\MNE-sample-data"
)
set "SampleDir=%MneSampleDataPath%\MEG\sample"

:: Input files
set "EvokedFile=%SampleDir%\sample_audvis-ave.fif"
set "InvFile=%SampleDir%\sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif"

:: Output directory
set "OutDir=%SampleDir%\processed"

:: Locate executable relative to this script
set "ScriptDir=%~dp0"
set "BuildPath=%ScriptDir%..\..\..\out\Release\apps\mne_compute_raw_inverse.exe"

:: Check if executable exists
if not exist "%BuildPath%" (
    echo Error: Could not locate mne_compute_raw_inverse executable at:
    echo   %BuildPath%
    echo Please build the project first.
    exit /b 1
)

:: Check if sample data exists
if not exist "%EvokedFile%" (
    echo Error: Sample data not found at:
    echo   %EvokedFile%
    echo Set MNE_DATA_PATH or download the MNE sample dataset to %%USERPROFILE%%\mne_data\
    exit /b 1
)

if not exist "%InvFile%" (
    echo Error: Inverse operator file not found at:
    echo   %InvFile%
    exit /b 1
)

:: Create output directory
mkdir "%OutDir%" 2>nul

echo ==============================================
echo  mne_compute_raw_inverse - MNE Sample Data
echo ==============================================
echo.
echo Evoked file  : %EvokedFile%
echo Inverse file : %InvFile%
echo Output dir   : %OutDir%
echo.

:: Run full source space dSPM inverse for all evoked sets
:: Parameters: SNR=3, dSPM, all sets, baseline -200..0 ms
echo --- Computing full source space dSPM inverse (all sets) ---
"%BuildPath%" ^
    --in "%EvokedFile%" ^
    --inv "%InvFile%" ^
    --snr 3 ^
    --spm ^
    --bmin -200 ^
    --bmax 0 ^
    --out "%OutDir%\sample_audvis-ave-spm"

echo.

echo Output files:
dir "%OutDir%\sample_audvis-ave-spm-*.stc"

echo.
echo Done.
endlocal
