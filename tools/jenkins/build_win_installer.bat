echo off
:: %0 Build Number
set arg0=%0

echo Starting MNE-CPP Windows Installer Build %arg0%

mkdir mne-cpp_installer_shadow_build
cd mne-cpp_installer_shadow_build

:: Visual Studio 2013
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

:: Visual Studio 2015
:: call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

:: -- Copy Files --
:: -MNE Lib-
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\bearer"
xcopy "..\mne-cpp\bin\bearer" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\bearer" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\iconengines"
xcopy "..\mne-cpp\bin\iconengines" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\iconengines" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\imageformats"
xcopy "..\mne-cpp\bin\imageformats" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\imageformats" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\platforms"
xcopy "..\mne-cpp\bin\platforms" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\platforms" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\translations"
xcopy "..\mne-cpp\bin\translations" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\translations" /s /e /y
xcopy "..\mne-cpp\bin\*.dll" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data\*.dll" /y
::xcopy "..\mne-cpp\bin\vcredist_x86.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data" /y
xcopy "..\mne-cpp\bin\vcredist_x64.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne\data" /y

:: -MNE Browse Raw Qt-
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data"
xcopy "..\mne-cpp\bin\mne_browse_raw_qt.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data" /y



:: Build Installer
qmake ../mne-cpp/tools/ifw_installer/windows/windows.pro -r
nmake clean
nmake

cd ..
copy ".\mne-cpp_installer_shadow_build\mne-cpp-windows-x86_64_1.0.0.exe" ".\"