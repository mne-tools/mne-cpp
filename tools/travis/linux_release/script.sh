#!/bin/bash

# Setup Qt environment, just verifying this is sourced before qmake or linuxdeployqt
source /opt/qt510/bin/qt510-env.sh

# Configure without tests
qmake -r MNECPP_CONFIG+=noTests

# Build
make -j2
