#!/bin/bash

# Install Qt 5.7 & Doxygen
sudo apt-get install -qq qt57base qt573d qt57svg qt57serialport qt57charts-no-lgpl

# Setup Qt environment
source /opt/qt57/bin/qt57-env.sh