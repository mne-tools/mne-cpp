# Common settings for mne-cpp build

MNE_CPP_VERSION = 0.1.0.252
MNE_LIB_VERSION = 1

# Paths
EIGEN_INCLUDE_DIR = $$EIGEN_INCLUDE_DIR
isEmpty(EIGEN_INCLUDE_DIR) {
    EIGEN_INCLUDE_DIR = $${PWD}/include/3rdParty
}

MNE_INCLUDE_DIR = $$MNE_INCLUDE_DIR
isEmpty( MNE_INCLUDE_DIR ) {
    MNE_INCLUDE_DIR = $${PWD}/MNE
}

MNE_LIBRARY_DIR = $$MNE_LIBRARY_DIR
isEmpty( MNE_LIBRARY_DIR ) {
    MNE_LIBRARY_DIR = $${PWD}/lib
}

#QT Packages use new qtHaveModule(<package>):
#MNE cpp config
MNECPP_CONFIG += isGui


QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2012 Authors of mne-cpp. All rights reserved.
