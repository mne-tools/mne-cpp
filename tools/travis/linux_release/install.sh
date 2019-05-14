#!/bin/bash

# Install Qt 5.10.1
#sudo apt-get install -qq qt510base qt5103d qt510svg qt510serialport qt510charts-no-lgpl

# Setup Qt environment
#source /opt/qt510/bin/qt510-env.sh



#Silent installer runs, installs qtcharts and qt_gcc

./qt-opensource-linux-x64-5.10.1.run --script qt5-noninteractive.qs

#recommended by BLFS editors for installing QT5

export QT5PREFIX=/opt/qt5

cat >> /etc/ld.so.conf.d/qt5.conf << "EOF"
# Adding qt5 libs

/opt/qt5/lib

# End addons 
EOF

#pulling in libs

ldconfig

#creating entry to path for profile.d 
cat > /etc/profile.d/qt5.sh << "EOF"
# Begin /etc/profile.d/qt5.sh

QT5DIR=/opt/qt5

pathappend $QT5DIR/bin           PATH
pathappend $QT5DIR/lib/pkgconfig PKG_CONFIG_PATH

export QT5DIR

# End /etc/profile.d/qt5.sh
EOF

