Rem This script needs to be run from the top level mne-cpp repo folder
# Solve for dependencies only mne_scan.exe and mnecppDisp3D.dll since it links all needed qt and mne-cpp libs
windeployqt .\bin\mne_scan.exe
windeployqt .\bin\mnecppDisp3D.dll
xcopy .\applications\mne_scan\plugins\brainflowboard\brainflow\installed\lib\* .\bin\ /s /i
xcopy .\applications\mne_scan\plugins\lsladapter\liblsl\build\install\bin\lsl.dll .\bin\ /i

# Delete folders which we do not want to ship
Remove-Item 'bin/mne-cpp-test-data' -Recurse

# Creating archive of all win deployed applications
7z a mne-cpp-windows-dynamic-x86_64.zip ./bin