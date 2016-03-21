:: ###startdir### %WORKSPACE%/mne-cpp/..

echo off
:: ### %0 Batch filename itself ###
set arg0=%0
:: ### %1 First command line parameter - suffix ###
set arg1=%1
if "%arg1%"=="" set arg1=default
set filename=mne-cpp-windows-x86_64-%arg1%.zip

echo Starting MNE-CPP Win Build; file name: %filename%

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

qmake ../mne-cpp/mne-cpp.pro -r
nmake clean
nmake

cd ..
del %filename%
call 7z a %filename% ./mne-cpp/bin