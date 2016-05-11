#!/bin/bash
set -ev

# Install Qt 5.6
sudo apt-get install -qq qt56base qt563d qt56svg qt56serialport
source /opt/qt56/bin/qt56-env.sh