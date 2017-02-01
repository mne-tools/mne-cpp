:: ###startdir### %WORKSPACE%/mne-cpp/..
echo off
:: ### %0 Batch filename itself ###
set arg0=%0

echo %arg0%

echo %APPVEYOR_REPO_BRANCH%

call .\tools\appveyor\windows_release\compress_binaries.bat %APPVEYOR_REPO_BRANCH%
call .\tools\appveyor\windows_release\build_installer.bat %APPVEYOR_REPO_BRANCH%
dir
