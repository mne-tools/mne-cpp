#!/bin/bash

# Install Qt 5.10.1
sudo apt-get install -qq qt510base qt5103d qt510svg qt510serialport qt510charts-no-lgpl cppcheck lcov 

# Setup Qt environment
source /opt/qt510/bin/qt510-env.sh

# Install Codecov
sudo pip install codecov
