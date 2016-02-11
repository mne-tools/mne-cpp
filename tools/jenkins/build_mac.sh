#!/bin/bash
# ===startdir=== %WORKSPACE%/mne-cpp/..
# %0 Build Number
# arg0=%0

echo Starting MNE-CPP Mac Build

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

QT_BIN_DIR='/Users/Shared/Jenkins/Qt5.6.0/5.6/clang_64/bin'
QT_LIB_DIR='/Users/Shared/Jenkins/Qt5.6.0/5.6/clang_64/lib'

TANGIBLES=(mne_x mne_browse_raw_qt mne_analyze_qt)

PATH=$QT_BIN_DIR:$PATH
export PATH

qmake ../mne-cpp/mne-cpp.pro -r
make clean
make -j4

# === Deployment ===
n_elements=${#TANGIBLES[@]}
for ((i = 0; i < n_elements; i++)); do
    tangible="../mne-cpp/bin/${TANGIBLES[i]}.app"
    macdeployqt $tangible
done
