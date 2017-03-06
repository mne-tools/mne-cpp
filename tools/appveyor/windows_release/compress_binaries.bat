echo off
:: ### %0 Batch filename itself ###
set arg0=%0
:: ### %1 First command line parameter - suffix ###
set arg1=%1
if "%arg1%"=="" set arg1=default
set filename=mne-cpp-windows-x86_64-%arg1%.zip

echo Start Compressing MNE-CPP Win Build; file name: %filename%

call 7z a %filename% ./bin