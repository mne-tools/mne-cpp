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

    # Deploy qt dependencies
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
    mne_binary_dir = $$2
    mne_library_dir = $$3
    extra_args = $$4

    file = $$shell_quote($$shell_path($${mne_library_dir}/$${target}.dll))
    final_deploy_command += $${QMAKE_COPY} $${file} $$shell_quote($${mne_binary_dir}) $$escape_expand(\\n\\t)

    # Deploy qt dependecies for the library
    deploy_target = $$shell_quote($$shell_path($${mne_binary_dir}/$${target}.dll))
    deploy_cmd = windeployqt

    final_deploy_command += $$deploy_cmd $$deploy_target $$extra_args $$escape_expand(\\n\\t)

    return($${final_deploy_command})
}

defineReplace(winDeployAppArgs) {
    target = $$1
    mne_binary_dir = $$2
    mne_library_dir = $$3
    extra_args = $$4

    # Deploy qt dependencies
    deploy_target = $$shell_quote($$shell_path($${mne_binary_dir}/$${target}.exe))
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

VERSION = 0.1.6

QMAKE_TARGET_PRODUCT = MNE-CPP
QMAKE_TARGET_DESCRIPTION = MNE-CPP Qt and Eigen based C++ library.
QMAKE_TARGET_COPYRIGHT = Copyright (C) 2020 Authors of MNE-CPP. All rights reserved.

########################################### PROJECT CONFIGURATION #############################################

## To compile with code coverage support run: qmake MNECPP_CONFIG+=withCodeCov
## To disable tests run: qmake MNECPP_CONFIG+=noTests
## To disable examples run: qmake MNECPP_CONFIG+=noExamples
## To disable applications run: qmake MNECPP_CONFIG+=noApplications
## To build MNE-CPP libraries and executables statically run: qmake MNECPP_CONFIG+=static
## To build MNE-CPP with FFTW support in Eigen (make sure to specify FFTW_DIRs below): qmake MNECPP_CONFIG+=useFFTW
## To build MNE-CPP Disp library without OpenGL support (default is with OpenGL support): qmake MNECPP_CONFIG+=noOpenGL
## To build MNE-CPP against WebAssembly (Wasm): qmake MNECPP_CONFIG+=wasm
## To build MNE Scan with BrainFlow support: qmake MNECPP_CONFIG+=withBrainFlow
## To build MNE Scan with LSL support: qmake MNECPP_CONFIG+=withLsl
## To build MNE Scan with BrainAMP support: qmake MNECPP_CONFIG+=withBrainAmp
## To build MNE Scan with EegoSports support: qmake MNECPP_CONFIG+=withEego
## To build MNE Scan with GUSBAmp support: qmake MNECPP_CONFIG+=withGUSBAmp
## To build MNE Scan with TMSI support: qmake MNECPP_CONFIG+=withTmsi

# Default flags
MNECPP_CONFIG +=

# Check versions
!minQtVersion(5, 10, 0) {
    error("You are trying to build with Qt version $${QT_VERSION}. However, the minimal Qt version to build MNE-CPP is 5.10.0.")
}

# Build static version if wasm flag was defined
contains(MNECPP_CONFIG, wasm) {
    message("The wasm flag was detected. Building static version of MNE-CPP. Disable OpenGL support for Disp library.")
    MNECPP_CONFIG += static noOpenGL
}

contains(MNECPP_CONFIG, static) {
    message("The static flag was detected. Building static version of MNE-CPP.")
}

########################################### DIRECTORY DEFINITIONS #############################################

# Eigen dir
EIGEN_INCLUDE_DIR = $$EIGEN_INCLUDE_DIR
isEmpty(EIGEN_INCLUDE_DIR) {
    EIGEN_INCLUDE_DIR = $$shell_path($${PWD}/include/3rdParty/eigen3)
}

# include dir
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
win32 {
    FFTW_DIR_LIBS = $$shell_path($${PWD}/include/3rdParty/fftw)
    FFTW_DIR_INCLUDE = $$shell_path($${PWD}/include/3rdParty/fftw)
}
unix {
    FFTW_DIR_LIBS = $$shell_path($${PWD}/include/3rdParty/fftw/lib)
    FFTW_DIR_INCLUDE = $$shell_path($${PWD}/include/3rdParty/fftw/include)
}
