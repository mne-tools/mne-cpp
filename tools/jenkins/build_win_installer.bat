echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Windows Installer Build %arg0%

mkdir mne-cpp_installer_shadow_build
cd mne-cpp_installer_shadow_build

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

qmake ../mne-cpp/tools/ifw_installer/windows/windows.pro -r
nmake clean
nmake
