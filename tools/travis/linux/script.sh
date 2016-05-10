#!/bin/bash
set -ev

# Configure and build
qmake ../mne-cpp/mne-cpp.pro -r
make -j2