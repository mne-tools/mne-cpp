#!/bin/bash

# Install Qt 5.10.1
sudo apt-get install -qq qt510base qt5103d qt510svg qt510serialport qt510charts-no-lgpl

# Setup Qt environment
source /opt/qt510/bin/qt510-env.sh