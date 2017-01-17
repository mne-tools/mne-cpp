############################################## GLOBAL FUNCTIONS ###############################################
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


############################################### GLOBAL DEFINES ################################################

MNE_CPP_VERSION = 1.0.0
MNE_LIB_VERSION = 1

QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2016 Authors of mne-cpp. All rights reserved.


########################################### PROJECT CONFIGURATION #############################################

## To build only the minimal version, i.e, for mne_rt_server run: qmake MNECPP_CONFIG+=minimalVersion
## To set CodeCov coverage compiler flag run: qmake MNECPP_CONFIG+=withCodeCov
## To disable tests run: qmake MNECPP_CONFIG+=noTests
## To disable examples run: qmake MNECPP_CONFIG+=noExamples
## To build basic MNE-X version run: qmake MNECPP_CONFIG+=BuildBasicMNESCANVersion
#MNECPP_CONFIG += BuildBasicMNESCANVersion

## Build MNE-CPP libraries as static libs
#MNECPP_CONFIG += build_MNECPP_Static_Lib

## Build MNE-CPP Deep library
MNECPP_CONFIG += buildDeep

#Build minimalVersion for qt versions <5.7.0
!minQtVersion(5, 7, 0) {
    message("Building minimal version due to Qt version $${QT_VERSION}.")
    MNECPP_CONFIG += minimalVersion
}


########################################### DIRECTORY DEFINITIONS #############################################

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
MNE_SCAN_INCLUDE_DIR = $$MNE_SCAN_INCLUDE_DIR
isEmpty( MNE_SCAN_INCLUDE_DIR ) {
    MNE_SCAN_INCLUDE_DIR = $${PWD}/applications/mne_scan/libs
}
contains(MNECPP_CONFIG, buildDeep) {
    CNTK_INCLUDE_DIR = $$MNE_INCLUDE_DIR
    isEmpty( CNTK_INCLUDE_DIR ) {
        CNTK_INCLUDE_DIR = C:/local/cntk/Include
    }
}

# lib
MNE_LIBRARY_DIR = $$MNE_LIBRARY_DIR
isEmpty( MNE_LIBRARY_DIR ) {
    MNE_LIBRARY_DIR = $${PWD}/lib
}
contains(MNECPP_CONFIG, buildDeep) {
    CNTK_LIBRARY_DIR = $$CNTK_LIBRARY_DIR
    isEmpty( CNTK_LIBRARY_DIR ) {
        CNTK_LIBRARY_DIR = C:/local/cntk/cntk
    }
}

# bin
MNE_BINARY_DIR = $$MNE_BINARY_DIR
isEmpty( MNE_BINARY_DIR ) {
    MNE_BINARY_DIR = $${PWD}/bin
}
