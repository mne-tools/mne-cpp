# Common settings for mne-cpp build

MNE_CPP_VERSION = 0.1.0.252
MNE_LIB_VERSION = 1

# Paths
# Eigen
EIGEN_INCLUDE_DIR = $$EIGEN_INCLUDE_DIR
isEmpty(EIGEN_INCLUDE_DIR) {
    EIGEN_INCLUDE_DIR = $${PWD}/include/3rdParty
}
# include
MNE_INCLUDE_DIR = $$MNE_INCLUDE_DIR
isEmpty( MNE_INCLUDE_DIR ) {
    MNE_INCLUDE_DIR = $${PWD}/MNE
}
MNE_X_INCLUDE_DIR = $$MNE_X_INCLUDE_DIR
isEmpty( MNE_X_INCLUDE_DIR ) {
    MNE_X_INCLUDE_DIR = $${PWD}/applications/mne_x_libs
}
# lib
MNE_LIBRARY_DIR = $$MNE_LIBRARY_DIR
isEmpty( MNE_LIBRARY_DIR ) {
    MNE_LIBRARY_DIR = $${PWD}/lib
}
# bin
MNE_BINARY_DIR = $$MNE_BINARY_DIR
isEmpty( MNE_BINARY_DIR ) {
    MNE_BINARY_DIR = $${PWD}/bin
}

#QT Packages use new qtHaveModule(<package>):
#MNE cpp config
MNECPP_CONFIG += isGui
#MNECPP_CONFIG += babyMEG

QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2012 Authors of mne-cpp. All rights reserved.

HEADERS += \
    ../../applications/mne_browse_raw_qt/types.h
