#!/bin/bash

# Configure without tests
# qmake -r MNECPP_CONFIG+=noTests

# Build
# make -j2

QMAKE_VERSION=`qmake -v`
QT_LIB_DIR=${QMAKE_VERSION##*in }
QT_BIN_DIR=$QT_LIB_DIR/../bin/
echo $QT_LIB_DIR
echo $QT_BIN_DIR
echo $PATH
