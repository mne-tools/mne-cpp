#!/bin/bash

# Configure without tests MNECPP_CONFIG+=noExamples MNECPP_CONFIG+=noTests
qmake -r MNECPP_CONFIG+=minimalVersion

# Build
make -j2
