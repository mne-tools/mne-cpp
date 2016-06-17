#!/bin/bash

# Configure without tests
qmake -r MNECPP_CONFIG+=minimalVersion

# Build
make -j2