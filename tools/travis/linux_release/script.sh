#!/bin/bash

# Configure without tests
qmake -qt=qt5 --version
qmake -r MNECPP_CONFIG+=noTests

# Build
make -j2

