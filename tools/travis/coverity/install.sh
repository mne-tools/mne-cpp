#!/bin/bash

# Install Qt 5.9.1
sudo apt-get install -qq qt59base qt593d qt59svg qt59serialport qt59charts-no-lgpl

# Setup Qt environment
source /opt/qt59/bin/qt59-env.sh