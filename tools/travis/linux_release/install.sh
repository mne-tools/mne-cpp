#!/bin/bash
set -ev

# Install Qt 5.6, cppcheck, lcov
sudo apt-get install -qq qt56base qt563d qt56svg qt56serialport cppcheck lcov
#source /opt/qt56/bin/qt56-env.sh

# Install Codecov
sudo pip install codecov
