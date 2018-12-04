echo off

:: Configure
qmake mne-cpp.pro -r MNECPP_CONFIG+=noApplications MNECPP_CONFIG+=noExamples
