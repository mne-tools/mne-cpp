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

################# MNE cpp config #################
## To build only the minimal version, i.e, for mne_rt_server run: qmake MNECPP_CONFIG+=minimalVersion
## To set CodeCov coverage compiler flag run: qmake MNECPP_CONFIG+=withCodeCov
## To disable tests run: qmake MNECPP_CONFIG+=noTests
## To disable examples run: qmake MNECPP_CONFIG+=noExamples

contains(MNECPP_CONFIG, coverity) {
    MNECPP_CONFIG += noTests
    MNECPP_CONFIG += noExamples
}

## To build basic MNE-X version run: qmake MNECPP_CONFIG+=BuildBasicMNEXVersion
#MNECPP_CONFIG += BuildBasicMNEXVersion

## Build MNE-CPP libraries as static libs
#MNECPP_CONFIG += build_MNECPP_Static_Lib


#Build minimalVersion for qt versions <5.7.0
!minQtVersion(5, 7, 0) {
    message("Building minimal version due to Qt version $${QT_VERSION}.")
    MNECPP_CONFIG += minimalVersion
}

QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2016 Authors of mne-cpp. All rights reserved.

#Define minQtVersion Test
defineTest(minQtVersion) {
    maj = $$1
    min = $$2
    patch = $$3
    isEqual(QT_MAJOR_VERSION, $$maj) {
        isEqual(QT_MINOR_VERSION, $$min) {
            isEqual(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
            greaterThan(QT_PATCH_VERSION, $$patch) {
                return(true)
            }
        }
        greaterThan(QT_MINOR_VERSION, $$min) {
            return(true)
        }
    }
    greaterThan(QT_MAJOR_VERSION, $$maj) {
        return(true)
    }
    return(false)
}
