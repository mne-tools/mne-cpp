#!/bin/bash

# Install Qt 5.6 & Doxygen
sudo apt-get install -qq qt56base qt563d qt56svg qt56serialport doxygen graphviz

# Setup Qt environment
source /opt/qt56/bin/qt56-env.sh
