Rem This script needs to be run from the top level mne-cpp repo folder
Rem Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
SET scriptPath=%~dp0
windeployqt %scriptPath%..\..\bin\mne_scan.exe
windeployqt %scriptPath%..\..\bin\mnecppDisp3D.dll

Rem Copy LSL and Brainflowlibraries manually
xcopy %scriptPath%..\..\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* %scriptPath%..\..\bin\ /s /i
xcopy %scriptPath%..\..\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %scriptPath%..\..\bin\ /i

Rem Delete folders which we do not want to ship
Remove-Item '%scriptPath%..\..\bin/mne-cpp-test-data' -Recurse

Rem Creating archive of all win deployed applications
7z a mne-cpp-windows-dynamic-x86_64.zip %scriptPath%..\../bin