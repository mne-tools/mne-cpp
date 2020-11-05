# This script needs to be run from the top level mne-cpp repo folder
# Delete folders which we do not want to ship
Remove-Item 'bin/mne_rt_server_plugins' -Recurse
Remove-Item 'bin/mne-cpp-test-data' -Recurse
Remove-Item 'bin/mne_scan_plugins' -Recurse
Remove-Item 'bin/mne_analyze_plugins' -Recurse

# Creating archive of everything in the bin directory
7z a mne-cpp-windows-static-x86_64.zip ./bin