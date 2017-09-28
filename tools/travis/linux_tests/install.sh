#!/bin/bash

# Install Qt 5.9.1, cppcheck, lcov
sudo apt-get install -qq qt59base qt593d qt59svg qt59serialport qt59charts-no-lgpl cppcheck lcov 

# Setup Qt environment
source /opt/qt59/bin/qt591-env.sh

# Install Codecov
sudo pip install codecov
