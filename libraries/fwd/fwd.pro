#--------------------------------------------------------------------------------------------------------------
#
# @file     fwd.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     January, 2017
#
# @section  LICENSE
#
# Copyright (C) 2017, Christoph Dinh and Matti Hamalainen. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that
# the following conditions are met:
#     * Redistributions of source code must retain the above copyright notice, this list of conditions and the
#       following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
#       the following disclaimer in the documentation and/or other materials provided with the distribution.
#     * Neither the name of MNE-CPP authors nor the names of its contributors may be used
#       to endorse or promote products derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#
# @brief    This project file builds the forward library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT += concurrent
QT -= gui

DEFINES += FWD_LIBRARY

TARGET = Fwd
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICLIB
} else {
    CONFIG += dll
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne
}

SOURCES += \
    computeFwd/compute_fwd_settings.cpp \
    computeFwd/compute_fwd.cpp \
    fwd_bem_model.cpp \
    fwd_bem_solution.cpp \
    fwd_coil.cpp \
    fwd_coil_set.cpp \
    fwd_comp_data.cpp \
    fwd_eeg_sphere_layer.cpp \
    fwd_eeg_sphere_model.cpp \
    fwd_eeg_sphere_model_set.cpp \
    fwd_thread_arg.cpp

HEADERS +=\
    fwd_global.h \
    computeFwd/compute_fwd_settings.h \
    computeFwd/compute_fwd.h \
    fwd_bem_model.h \
    fwd_bem_solution.h \
    fwd_coil.h \
    fwd_coil_set.h \
    fwd_comp_data.h \
    fwd_eeg_sphere_layer.h \
    fwd_eeg_sphere_model.h \
    fwd_eeg_sphere_model_set.h \
    fwd_thread_arg.h \
    fwd_types.h

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/general/surf2bem/icos.fif \
    $${ROOT_DIR}/resources/general/coilDefinitions/coil_def.dat \
    $${ROOT_DIR}/resources/general/coilDefinitions/coil_def_Elekta.dat \

# Copy resource files from repository to bin resource folder
COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/fwd

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

DISTFILES += \
    dipoleFit/dipolefit_helpers_bak.txt

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

# OpenMP
win32 {
    QMAKE_CXXFLAGS  +=  -openmp
    #QMAKE_LFLAGS    +=  -openmp
}
unix:!macx {
    QMAKE_CXXFLAGS  +=  -fopenmp
    QMAKE_LFLAGS    +=  -fopenmp
}

# Deploy library in non-static builds only
win32:!contains(MNECPP_CONFIG, static) {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${EXTRA_ARGS})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}

# Activate FFTW backend in Eigen for non-static builds only
contains(MNECPP_CONFIG, useFFTW):!contains(MNECPP_CONFIG, static) {
    DEFINES += EIGEN_FFTW_DEFAULT
    INCLUDEPATH += $$shell_path($${FFTW_DIR_INCLUDE})
    LIBS += -L$$shell_path($${FFTW_DIR_LIBS})

    win32 {
        # On Windows
        LIBS += -llibfftw3-3 \
                -llibfftw3f-3 \
                -llibfftw3l-3 \
    }

    unix:!macx {
        # On Linux
        LIBS += -lfftw3 \
                -lfftw3_threads \
    }
}


