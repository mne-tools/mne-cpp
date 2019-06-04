#!/bin/bash

# Install Qt 5.10.1
sudo apt-get install -qq qt510base qt5103d qt510svg qt510serialport qt510charts-no-lgpl qt510-meta-minimal qt510imageformats

# Install non-QT packages, just paranoia.
sudo apt-get install -qq libicu52 libxcb-xinerama0

# Setup Qt environment
source /opt/qt510/bin/qt510-env.sh

# Move azcopy so we can easily use it
tar -xzvf downloadazcopy-v10-linux
find . -type f -name 'azcopy' -exec mv -t ./ {} +
