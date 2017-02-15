echo off

:: Configure without tests
qmake mne-cpp.pro -r MNECPP_CONFIG+=noTests
