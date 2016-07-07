#!/bin/bash

# Install Qt 5.7, cppcheck, lcov
sudo apt-get install -qq qt57base qt573d qt57svg qt57serialport qt57charts-no-lgpl cppcheck lcov 

# Setup Qt environment
source /opt/qt57/bin/qt57-env.sh

# Install Codecov
sudo pip install codecov
