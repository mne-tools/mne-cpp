echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Win Build %arg1%

qmake "../../mne-cpp.pro" -recursive

nmake