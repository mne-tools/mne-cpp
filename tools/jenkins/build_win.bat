echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Win Build %arg1%

mkdir MNE-CPP_shadow_build
cd MNE-CPP_shadow_build

qmake ../../../mne-cpp.pro -recursive

nmake