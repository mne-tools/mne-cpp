echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Win Build %arg1%

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

qmake ../mne-cpp/mne-cpp.pro -r
:: nmake clean
nmake

cd ..
del mne-cpp_win.zip
call 7z a mne-cpp_win.zip ./mne-cpp/bin