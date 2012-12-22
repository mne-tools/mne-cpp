#Common settings for mne-cp build

MNE_CPP_VERSION = 0.1.0.252

MNE_LIB_VERSION = 1

#Paths
EIGEN_HOME = include/3rdParty
MNE_HOME = MNE



isGui = true #false
isGui {
    Qt3D_available = false #true
}


QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2012 Authors of mne-cpp. All rights reserved.
