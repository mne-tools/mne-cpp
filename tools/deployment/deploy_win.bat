SET scriptPath=%~dp0
Rem Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
windeployqt %scriptPath%..\..\bin\mne_scan.exe
windeployqt %scriptPath%..\..\bin\mnecppDisp3D.dll

Rem Copy LSL and Brainflowlibraries manually
xcopy %scriptPath%..\..\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* %scriptPath%..\..\bin\ /s /i
xcopy %scriptPath%..\..\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll %scriptPath%..\..\bin\ /i


