#!/bin/bash

# Install Qt 5.8, cppcheck, lcov
sudo apt-get install -qq qt58base qt583d qt58svg qt58serialport qt58charts-no-lgpl cppcheck lcov 

# Setup Qt environment
source /opt/qt58/bin/qt58-env.sh

# Install Codecov
sudo pip install codecov
