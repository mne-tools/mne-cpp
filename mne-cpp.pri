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

defineReplace(macDeployArgs) {
    target = $$1
    target_ext = $$2
    mne_binary_dir = $$3
    mne_library_dir = $$4
    extra_args = $$5

    isEmpty(target_ext) {
        target_ext = .app
    }

    deploy_cmd = macdeployqt

    deploy_target = $$shell_quote($$shell_path($${mne_binary_dir}/$${target}$${target_ext}))

    deploy_libs_to_copy = -libpath=$${mne_library_dir}
    !isEmpty(extra_args) {
      deploy_libs_to_copy += $${extra_args}
    }
    return($$deploy_cmd $$deploy_target $$deploy_libs_to_copy)
}

defineReplace(winDeployLibArgs) {
    # Copy library to bin folder
    target = $$1
    target_ext = $$2
    mne_binary_dir = $$3
    mne_library_dir = $$4
    extra_args = $$5

    isEmpty(target_ext) {
        target_ext = .dll
    }

    file = $$shell_quote($$shell_path($${mne_library_dir}/$${target}$${target_ext}))
    final_deploy_command += $${QMAKE_COPY} $${file} $$shell_quote($${mne_binary_dir}) $$escape_expand(\\n\\t)

    # Deploy qt dependecies for the library
    deploy_target = $$shell_quote($$shell_path($${mne_binary_dir}/$${target}$${target_ext}))
    deploy_cmd = windeployqt

    final_deploy_command += $$deploy_cmd $$deploy_target $$extra_args $$escape_expand(\\n\\t)

    return($${final_deploy_command})
}

defineReplace(winDeployAppArgs) {
    target = $$1
    target_ext = $$2
    mne_binary_dir = $$3
    extra_args = $$5

    # Deploy qt dependecies for the application
    isEmpty(target_ext) {
        target_ext = .exe
    }

    deploy_target = $$shell_quote($$shell_path($${mne_binary_dir}/$${target}$${target_ext}))
    deploy_cmd = windeployqt

    final_deploy_command += $$deploy_cmd $$deploy_target $$extra_args $$escape_expand(\\n\\t)

    return($${final_deploy_command})
}

defineReplace(copyResources) {
    resource_files = $$1

    for(FILE, resource_files) {
        FILEDIR = $$dirname(FILE)
        FILEDIR ~= s,/resources,/bin/resources,g
        FILEDIR = $$shell_path($${FILEDIR})
        TRGTDIR = $${FILEDIR}

        final_copy_command += $$sprintf($${QMAKE_MKDIR_CMD}, "$${TRGTDIR}") $$escape_expand(\n\t)

        FILE = $$shell_path($${FILE})
        final_copy_command += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${TRGTDIR}) $$escape_expand(\\n\\t)
    }

    return($${final_copy_command})
}


############################################### GLOBAL DEFINES ################################################

MNE_CPP_VERSION = 1.0.0
MNE_LIB_VERSION = 1

QMAKE_TARGET_PRODUCT = mne-cpp
QMAKE_TARGET_DESCRIPTION = MNE Qt 5 based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2019 Authors of mne-cpp. All rights reserved.


########################################### PROJECT CONFIGURATION #############################################

## To build only the minimal version, i.e, for mne_rt_server run: qmake MNECPP_CONFIG+=minimalVersion
## To set CodeCov coverage compiler flag run: qmake MNECPP_CONFIG+=withCodeCov
## To disable tests run: qmake MNECPP_CONFIG+=noTests
## To disable examples run: qmake MNECPP_CONFIG+=noExamples
## To disable applications run: qmake MNECPP_CONFIG+=noApplications
## To build basic MNE Scan version run: qmake MNECPP_CONFIG+=buildBasicMneScanVersion
## To build MNE-CPP libraries as static libs: qmake MNECPP_CONFIG+=static
## To build MNE-CPP Deep library based CNTK: qmake MNECPP_CONFIG+=buildDeep
## To build MNE-CPP with FFTW support in Eigen (make sure to specify FFTW_DIRs below): qmake MNECPP_CONFIG+=useFFTW
## To build MNE-CPP Disp library with OpenGL support (default is with OpenGL support): qmake MNECPP_CONFIG+=dispOpenGL

# Default flags
MNECPP_CONFIG += dispOpenGL

#Build minimalVersion for qt versions < 5.10.0
!minQtVersion(5, 10, 0) {
    message("Building minimal version due to Qt version $${QT_VERSION}.")
    MNECPP_CONFIG += minimalVersion
}


########################################### DIRECTORY DEFINITIONS #############################################

# Eigen
EIGEN_INCLUDE_DIR = $$EIGEN_INCLUDE_DIR
isEmpty(EIGEN_INCLUDE_DIR) {
    EIGEN_INCLUDE_DIR = $$shell_path($${PWD}/include/3rdParty/eigen3)
}

#CNTK
CNTK_INCLUDE_DIR = $$CNTK_INCLUDE_DIR
isEmpty( CNTK_INCLUDE_DIR ) {
    # Check CNTK Path options
    exists($$shell_path($$(CNTKPATH)/cntk/Include/Eval.h)) {
        CNTK_TEST_DIR = $$shell_path($$(CNTKPATH)/cntk)
    }
    exists($$shell_path($$(CNTKPATH)/Include/Eval.h)) {
        CNTK_TEST_DIR = $$(CNTKPATH)
    }
    exists($$shell_path($$(MYCNTKPATH)/cntk/Include/Eval.h)) {
        CNTK_TEST_DIR = $$shell_path($$(MYCNTKPATH)/cntk)
    }
    exists($$shell_path($$(MYCNTKPATH)/Include/Eval.h)) {
        CNTK_TEST_DIR = $$(MYCNTKPATH)
    }
    # Set CNTK path variables
    !isEmpty( CNTK_TEST_DIR ) {
        CNTK_INCLUDE_DIR = $$shell_path($${CNTK_TEST_DIR}/Include)
        CNTK_LIBRARY_DIR = $$shell_path($${CNTK_TEST_DIR}/cntk)
    }
}

# include
MNE_INCLUDE_DIR = $$MNE_INCLUDE_DIR
isEmpty( MNE_INCLUDE_DIR ) {
    MNE_INCLUDE_DIR = $$shell_path($${PWD}/libraries)
}
MNE_SCAN_INCLUDE_DIR = $$MNE_SCAN_INCLUDE_DIR
isEmpty( MNE_SCAN_INCLUDE_DIR ) {
    MNE_SCAN_INCLUDE_DIR = $$shell_path($${PWD}/applications/mne_scan/libs)
}
MNE_ANALYZE_INCLUDE_DIR = $$MNE_ANALYZE_INCLUDE_DIR
isEmpty( MNE_ANALYZE_INCLUDE_DIR ) {
    MNE_ANALYZE_INCLUDE_DIR = $$shell_path($${PWD}/applications/mne_analyze/libs)
}
MNE_ANALYZE_EXTENSIONS_DIR = $$MNE_ANALYZE_EXTENSIONS_DIR
isEmpty( MNE_ANALYZE_EXTENSIONS_DIR ) {
    MNE_ANALYZE_EXTENSIONS_DIR = $$shell_path($${PWD}/applications/mne_analyze/extensions)
}

# lib
MNE_LIBRARY_DIR = $$MNE_LIBRARY_DIR
isEmpty( MNE_LIBRARY_DIR ) {
    MNE_LIBRARY_DIR = $$shell_path($${PWD}/lib)
}
contains(MNECPP_CONFIG, buildDeep) {
    CNTK_LIBRARY_DIR = $$CNTK_LIBRARY_DIR
    isEmpty( CNTK_LIBRARY_DIR ) {
        CNTK_LIBRARY_DIR = $$shell_path(C:/local/cntk/cntk)
    }
}

# bin
MNE_BINARY_DIR = $$MNE_BINARY_DIR
isEmpty( MNE_BINARY_DIR ) {
    MNE_BINARY_DIR = $$shell_path($${PWD}/bin)
}

# repository dir
ROOT_DIR = $${PWD}

# install
MNE_INSTALL_INCLUDE_DIR = $$MNE_INSTALL_INCLUDE_DIR
isEmpty( MNE_INSTALL_INCLUDE_DIR ) {
    MNE_INSTALL_INCLUDE_DIR = $$shell_path($${PWD}/include)
}

# FFTW dir
FFTW_DIR_LIBS = $$shell_path(K:\fftw-3.3.5-dll64)
FFTW_DIR_INCLUDE = $$shell_path(K:\fftw-3.3.5-dll64)
