#!/bin/bash

# Configure without tests
qmake -r MNECPP_CONFIG+=noTests

# Build
make -j2
