echo off

echo Current branch is: %APPVEYOR_REPO_BRANCH%

:: Call artifact creation scripts
call .\tools\appveyor\windows_release\compress_binaries.bat %APPVEYOR_REPO_BRANCH%
call .\tools\appveyor\windows_release\build_installer.bat %APPVEYOR_REPO_BRANCH%
dir
