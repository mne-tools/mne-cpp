SET scriptPath=%~dp0
# Delete folders which we do not want to ship
Remove-Item '%scriptPath%..\..\bin/mne_rt_server_plugins' -Recurse
Remove-Item '%scriptPath%..\..\bin/mne-cpp-test-data' -Recurse
Remove-Item '%scriptPath%..\..\bin/mne_scan_plugins' -Recurse
Remove-Item '%scriptPath%..\..\bin/mne_analyze_plugins' -Recurse

# Creating archive of everything in the bin directory
7z a mne-cpp-windows-static-x86_64.zip %scriptPath%..\../bin