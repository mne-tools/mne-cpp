:: ###startdir### %WORKSPACE%/mne-cpp/..
echo off
:: ### %0 Batch filename itself ###
set arg0=%0

echo %arg0%

qmake mne-cpp.pro -r MNECPP_CONFIG+=noTests
