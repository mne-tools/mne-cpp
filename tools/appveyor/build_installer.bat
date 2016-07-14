:: ###startdir### %WORKSPACE%/mne-cpp/..

echo off
:: ### %0 Batch filename itself ###
set arg0=%0
:: ### %1 First command line parameter - suffix ###
set arg1=%1
if "%arg1%"=="" set arg1=default
set filename=mne-cpp-windows-x86_64-%arg1%.exe

echo Start MNE-CPP Windows Installer Build; file name: %filename%

:: -- Copy Files --
:: -MNE Lib-
rmdir ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /s /q
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\bearer"
xcopy ".\bin\bearer" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\bearer" /s /e /y
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\iconengines"
xcopy ".\bin\iconengines" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\iconengines" /s /e /y
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\imageformats"
xcopy ".\bin\imageformats" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\imageformats" /s /e /y
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\platforms"
xcopy ".\bin\platforms" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\platforms" /s /e /y
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\translations"
xcopy ".\bin\translations" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\translations" /s /e /y
xcopy ".\bin\*.dll" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data\*.dll" /y
::xcopy ".\bin\vcredist_x86.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /y
xcopy ".\bin\vcredist_x64.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite\data" /y

:: -MNE-X-
rmdir ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /s /q
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data"
::mne_x
xcopy ".\bin\mne_x.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy ".\bin\xShared.dll" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy ".\bin\xDisp.dll" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
xcopy ".\bin\xMeas.dll" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
::mne_x libs
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_libs"
xcopy ".\bin\mne_x_libs" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_libs" /s /e /y
::mne_x plugins
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_plugins"
xcopy ".\bin\mne_x_plugins" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_x_plugins" /s /e /y
::mne_rt_server
xcopy ".\bin\mne_rt_server.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data" /y
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_rt_server_plugins"
xcopy ".\bin\mne_rt_server_plugins" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_x\data\mne_rt_server_plugins" /s /e /y

:: -MNE Browse Raw Qt-
rmdir ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data" /s /q
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data"
xcopy ".\bin\mne_browse_raw_qt.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_browse_raw_qt\data" /y

:: -MNE Analyze Qt-
rmdir ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze\data" /s /q
mkdir -p ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze\data"
xcopy ".\bin\mne_analyze.exe" ".\tools\ifw_installer\windows\packages\org.mne_cpp.suite.mne_analyze\data" /y


:: Build Installer
qmake ./tools/ifw_installer/windows/windows.pro -r
nmake clean
nmake

ren mne-cpp-windows-x86_64.exe %filename%