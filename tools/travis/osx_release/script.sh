#!/bin/bash

# Configure without tests
# qmake -r MNECPP_CONFIG+=noTests

# Build
# make -j2

qmake -v
which qmake

QT_BIN_DIR=`qmake -v`

echo ${QT_BIN_DIR##*in }
echo $QT_BIN_DIR
