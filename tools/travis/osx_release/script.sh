#!/bin/bash

# Configure without tests
# qmake -r MNECPP_CONFIG+=noTests

# Build
# make -j2

QMAKE_VERSION=`qmake -v`
QT_LIB_DIR=${QMAKE_VERSION##*in }
QT_BIN_DIR=${QT_LIB_DIR%/*}/bin

MNE_LIB_DIR=$(pwd)/lib

PATH=$QT_BIN_DIR:$PATH
export PATH

DYLD_LIBRARY_PATH=$MNE_LIB_DIR:$DYLD_LIBRARY_PATH
export DYLD_LIBRARY_PATH
DYLD_FALLBACK_LIBRARY_PATH=$MNE_LIB_DIR
export DYLD_FALLBACK_LIBRARY_PATH


# === Deployment ===
TANGIBLES=(mne_x mne_browse_raw_qt mne_analyze_qt)
n_elements=${#TANGIBLES[@]}
for ((i = 0; i < n_elements; i++)); do
    fixfile="./bin/${TANGIBLES[i]}.app/Contents/MacOS/${TANGIBLES[i]}"
    destdir="./bin/${TANGIBLES[i]}.app/Contents/Frameworks/"

	dylibbundler -od -b -x $fixfile -d $destdir -p @executable_path/../Frameworks/
	
    tangible="./bin/${TANGIBLES[i]}.app"
    macdeployqt $tangible -dmg
done