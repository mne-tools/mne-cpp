#!/bin/bash
set -ev

# Install Qt 5.6, cppcheck, lcov
sudo apt-get install -qq qt56base qt563d qt56svg qt56serialport cppcheck lcov

# Define Qt environment explicitly instead of "source /opt/qt56/bin/qt56-env.sh" 
QT_BASE_DIR=/opt/qt56
export QTDIR=$QT_BASE_DIR
export PATH=$QT_BASE_DIR/bin:$PATH
if [[ $(uname -m) == "x86_64" ]]; then
  export LD_LIBRARY_PATH=$QT_BASE_DIR/lib/x86_64-linux-gnu:$QT_BASE_DIR/lib:$LD_LIBRARY_PATH
else
  export LD_LIBRARY_PATH=$QT_BASE_DIR/lib/i386-linux-gnu:$QT_BASE_DIR/lib:$LD_LIBRARY_PATH
fi
uname -m
export PKG_CONFIG_PATH=$QT_BASE_DIR/lib/pkgconfig:$PKG_CONFIG_PATH

# Install Codecov
sudo pip install codecov
