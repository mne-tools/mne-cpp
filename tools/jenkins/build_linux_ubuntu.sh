#!/bin/bash
# ===startdir=== %WORKSPACE%/mne-cpp/..
# %0 Build Number
# arg0=%0

echo Starting MNE-CPP Linux Ubuntu Build

mkdir mne-cpp_shadow_build
cd mne-cpp_shadow_build

QT_BIN_DIR='/opt/Qt5.6.0/5.6/gcc_64/bin'
QT_LIB_DIR='/opt/Qt5.6.0/5.6/gcc_64/lib'

QT_LIBS=(libQt5Core libQt5Gui libQt5Concurrent)

$QT_BIN_DIR/qmake ../mne-cpp/mne-cpp.pro -r
make clean
make -j4


# === Copy Libs ===
n_elements=${#QT_LIBS[@]}
for ((i = 0; i < n_elements; i++)); do
    libpath="$QT_LIB_DIR/${QT_LIBS[i]}.*"
    cp $libpath ../mne-cpp/lib
done


# === user package ===
cd ..
rm mne-cpp-linux-x86_64-1.0.0-beta.tar.gz
tar cfvz mne-cpp-linux-x86_64-1.0.0-beta.tar.gz ./mne-cpp/bin ./mne-cpp/lib
