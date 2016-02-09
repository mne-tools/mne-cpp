#!/bin/bash
# ===startdir=== %WORKSPACE%/mne-cpp/..
# %0 Build Number
# arg0=%0

echo Starting MNE-CPP Mac Build

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

QT_BIN_DIR='/Users/Shared/Jenkins/Qt5.6.0/5.6/clang_64/bin'
QT_LIB_DIR='/Users/Shared/Jenkins/Qt5.6.0/5.6/clang_64/lib'

$QT_BIN_DIR/qmake ../mne-cpp/mne-cpp.pro -r
make clean
make -j4
