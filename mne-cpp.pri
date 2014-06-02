# Common settings for mne-cpp build

MNE_CPP_VERSION = 0.1.0.252
MNE_LIB_VERSION = 1

# Paths
# Eigen
EIGEN_INCLUDE_DIR = $$EIGEN_INCLUDE_DIR
isEmpty(EIGEN_INCLUDE_DIR) {
    EIGEN_INCLUDE_DIR = $${PWD}/include/3rdParty/eigen3
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
MNECPP_CONFIG += withGui
#MNECPP_CONFIG += withPython

contains(MNECPP_CONFIG, withPython) {
    message(Configure Python!)

    PYTHON_DIR = $$system(python $${PWD}/tools/pytools/python.pwd.py)

    # include
    PYTHON_INCLUDE_DIR = $$PYTHON_INCLUDE_DIR
    isEmpty( PYTHON_INCLUDE_DIR ) {
        PYTHON_INCLUDE_DIR = $$system(python $${PWD}/tools/pytools/python.include.py)
    }

    # lib
    PYTHON_LIBRARY_DIR = $$PYTHON_LIBRARY_DIR
    isEmpty( PYTHON_LIBRARY_DIR ) {
        PYTHON_LIBRARY_DIR = $${PYTHON_DIR}/libs
    }
}

QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2014 Authors of mne-cpp. All rights reserved.
