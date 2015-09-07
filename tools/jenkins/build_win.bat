echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Win Build %arg1%

mkdir MNE-CPP_shadow_build
cd MNE-CPP_shadow_build


call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

qmake ../../../mne-cpp.pro -r

nmake