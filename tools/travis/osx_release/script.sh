#!/bin/bash

# Configure without tests
# qmake -r MNECPP_CONFIG+=noTests

# Build
# make -j2

QMAKE_VERSION=`qmake -v`
QT_LIB_DIR=${QMAKE_VERSION##*in }
QT_BIN_DIR=${QT_LIB_DIR%/*}/bin

PATH=$QT_BIN_DIR:$PATH
export PATH

MNE_LIB_DIR=$(pwd)/lib

echo $MNE_LIB_DIR
echo $PATH


TANGIBLES=(mne_x mne_browse_raw_qt mne_analyze_qt)