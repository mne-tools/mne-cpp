:: ###startdir### %WORKSPACE%/mne-cpp/..

echo off
:: ### %0 Batch filename itself ###
set arg0=%0
:: ### %1 First command line parameter - suffix ###
set arg1=%1
if "%arg1%"=="" set arg1=default
set filename=mne-cpp-windows-x86_64-%arg1%.exe

echo Starting MNE-CPP Windows Installer Build; file name: %filename%

mkdir mne-cpp_installer_shadow_build
cd mne-cpp_installer_shadow_build

:: Visual Studio 2013
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" amd64

:: Visual Studio 2015
:: call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

:: -- Copy Files --
:: -MNE Lib-
rmdir "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /s /q
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\bearer"
xcopy "..\mne-cpp\bin\bearer" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\bearer" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\iconengines"
xcopy "..\mne-cpp\bin\iconengines" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\iconengines" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\imageformats"
xcopy "..\mne-cpp\bin\imageformats" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\imageformats" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\platforms"
xcopy "..\mne-cpp\bin\platforms" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\platforms" /s /e /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\translations"
xcopy "..\mne-cpp\bin\translations" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\translations" /s /e /y
xcopy "..\mne-cpp\bin\*.dll" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\*.dll" /y
::xcopy "..\mne-cpp\bin\vcredist_x86.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /y
xcopy "..\mne-cpp\bin\vcredist_x64.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /y

:: -MNE-X-
rmdir "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /s /q
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data"
::mne_x
xcopy "..\mne-cpp\bin\mne_x.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy "..\mne-cpp\bin\mne_x.dll" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy "..\mne-cpp\bin\xDisp.dll" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy "..\mne-cpp\bin\xMeas.dll" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
::mne_x libs
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_libs"
xcopy "..\mne-cpp\bin\mne_x_libs" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_libs" /s /e /y
::mne_x plugins
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_plugins"
xcopy "..\mne-cpp\bin\mne_x_plugins" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_plugins" /s /e /y
::mne_rt_server
xcopy "..\mne-cpp\bin\mne_rt_server.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_rt_server_plugins"
xcopy "..\mne-cpp\bin\mne_rt_server_plugins" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_rt_server_plugins" /s /e /y

:: -MNE Browse Raw Qt-
rmdir "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data" /s /q
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data"
xcopy "..\mne-cpp\bin\mne_browse_raw_qt.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data" /y

:: -MNE Analyze Qt-
rmdir "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze_qt\data" /s /q
mkdir -p "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze_qt\data"
xcopy "..\mne-cpp\bin\mne_analyze_qt.exe" "..\mne-cpp\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze_qt\data" /y


:: Build Installer
qmake ../mne-cpp/tools/ifw_installer/windows/windows.pro -r
nmake clean
nmake

cd ..
cd ".\mne-cpp_installer_shadow_build\"
ren mne-cpp-windows-x86_64.exe %filename%
cd ..
copy ".\mne-cpp_installer_shadow_build\%filename%" ".\"