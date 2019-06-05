#!/bin/bash

# Setup Qt environment
source /opt/qt510/bin/qt510-env.sh

# Configure without tests
qmake -r MNECPP_CONFIG+=noTests

# Build
make -j2
