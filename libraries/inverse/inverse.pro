#--------------------------------------------------------------------------------------------------------------
#
# @file     inverse.pro
# @author   Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>
# @version  1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2012, Christoph Dinh and Matti Hamalainen. All rights reserved.
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
# @brief    This project file builds the inverse library.
#
#--------------------------------------------------------------------------------------------------------------

include(../../mne-cpp.pri)

TEMPLATE = lib

QT -= gui
QT += concurrent

DEFINES += INVERSE_LIBRARY

TARGET = Inverse
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
            -lMNE$${MNE_LIB_VERSION}Mned \
            -lMNE$${MNE_LIB_VERSION}Fwdd
} else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne \
            -lMNE$${MNE_LIB_VERSION}Fwd
}

SOURCES += \
    minimumNorm/minimumnorm.cpp \
    rapMusic/rapmusic.cpp \
    rapMusic/pwlrapmusic.cpp \
    rapMusic/dipole.cpp \
    dipoleFit/dipole_fit.cpp \
    dipoleFit/dipole_fit_data.cpp \
    dipoleFit/dipole_fit_settings.cpp \
    dipoleFit/dipole_forward.cpp \
    dipoleFit/ecd.cpp \
    dipoleFit/ecd_set.cpp \
    dipoleFit/guess_data.cpp \
    c/mne_inverse_operator.cpp \
    c/mne_meas_data.cpp \
    c/mne_meas_data_set.cpp \
    hpiFit/hpifit.cpp \
    hpiFit/hpifitdata.cpp


HEADERS +=\
    inverse_global.h \
    IInverseAlgorithm.h \
    minimumNorm/minimumnorm.h \
    rapMusic/rapmusic.h \
    rapMusic/pwlrapmusic.h \
    rapMusic/dipole.h \
    dipoleFit/analyze_types.h \
    dipoleFit/dipole_fit.h \
    dipoleFit/dipole_fit_data.h \
    dipoleFit/dipole_fit_settings.h \
    dipoleFit/dipole_forward.h \
    dipoleFit/ecd.h \
    dipoleFit/ecd_set.h \
    dipoleFit/guess_data.h \
    c/mne_inverse_operator.h \
    c/mne_meas_data.h \
    c/mne_meas_data_set.h \
    hpiFit/hpifit.h \
    hpiFit/hpifitdata.h

RESOURCE_FILES +=\
    $${ROOT_DIR}/resources/general/coilDefinitions/coil_def.dat \
    $${ROOT_DIR}/resources/general/coilDefinitions/coil_def_Elekta.dat \

# Copy resource files from repository to bin resource folder
COPY_CMD = $$copyResources($${RESOURCE_FILES})
QMAKE_POST_LINK += $${COPY_CMD}

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = $${HEADERS}
header_files.path = $${MNE_INSTALL_INCLUDE_DIR}/inverse

INSTALLS += header_files

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

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

# Deploy library
win32 {
    EXTRA_ARGS =
    DEPLOY_CMD = $$winDeployLibArgs($${TARGET},$${TARGET_EXT},$${MNE_BINARY_DIR},$${MNE_LIBRARY_DIR},$${MNECPP_CONFIG})
    QMAKE_POST_LINK += $${DEPLOY_CMD}
}

# Activate FFTW backend in Eigen
contains(MNECPP_CONFIG, useFFTW) {
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

