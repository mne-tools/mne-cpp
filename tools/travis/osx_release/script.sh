#!/bin/bash

# Configure without tests
# qmake -r MNECPP_CONFIG+=noTests

# Build
# make -j2

qmake -v
which qmake

QT_BIN_DIR=`which qmake`

echo "QT_BIN_DIR="
echo $QT_BIN_DIR
