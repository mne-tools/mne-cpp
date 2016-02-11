:: ###startdir### %WORKSPACE%/mne-cpp/..

echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Win Build %arg0%

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

qmake ../mne-cpp/mne-cpp.pro -r
nmake clean
nmake

cd ..
del mne-cpp-windows-x86_64-1.0.0-beta.zip
call 7z a mne-cpp-windows-x86_64-1.0.0-beta.zip ./mne-cpp/bin