#--------------------------------------------------------------------------------------------------------------
#
# @file     disp.pro
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

QT       -= gui

DEFINES += INVERSE_LIBRARY

TARGET = Inverse
TARGET = $$join(TARGET,,MNE$${MNE_LIB_VERSION},)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lMNE$${MNE_LIB_VERSION}Genericsd \
            -lMNE$${MNE_LIB_VERSION}Utilsd \
            -lMNE$${MNE_LIB_VERSION}Fsd \
            -lMNE$${MNE_LIB_VERSION}Fiffd \
            -lMNE$${MNE_LIB_VERSION}Mned
}
else {
    LIBS += -lMNE$${MNE_LIB_VERSION}Generics \
            -lMNE$${MNE_LIB_VERSION}Utils \
            -lMNE$${MNE_LIB_VERSION}Fs \
            -lMNE$${MNE_LIB_VERSION}Fiff \
            -lMNE$${MNE_LIB_VERSION}Mne
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

DESTDIR = $${MNE_LIBRARY_DIR}

contains(MNECPP_CONFIG, build_MNECPP_Static_Lib) {
    CONFIG += staticlib
    DEFINES += BUILD_MNECPP_STATIC_LIB
}
else {
    CONFIG += dll

    #
    # win32: copy dll's to bin dir
    # unix: add lib folder to LD_LIBRARY_PATH
    #
    win32 {
        FILE = $${DESTDIR}/$${TARGET}.dll
        BINDIR = $${DESTDIR}/../bin
        FILE ~= s,/,\\,g
        BINDIR ~= s,/,\\,g
        QMAKE_POST_LINK += $${QMAKE_COPY} $$quote($${FILE}) $$quote($${BINDIR}) $$escape_expand(\\n\\t)
    }
}

SOURCES += \
    dipoleFit/ecd.cpp \
    dipoleFit/ecd_set.cpp \
    dipoleFit/guess_data.cpp \
    dipoleFit/dipole_forward.cpp \
    dipoleFit/dipole_fit_data.cpp \
    dipoleFit/fwd_eeg_sphere_layer.cpp \
    dipoleFit/fwd_eeg_sphere_model.cpp \
    dipoleFit/fwd_eeg_sphere_model_set.cpp \
    dipoleFit/fwd_coil.cpp \
    dipoleFit/fwd_coil_set.cpp \
    minimumNorm/minimumnorm.cpp \
    rapMusic/rapmusic.cpp \
    rapMusic/pwlrapmusic.cpp \
    rapMusic/dipole.cpp \
    dipoleFit/dipole_fit_settings.cpp \
    dipoleFit/dipole_fit.cpp \
    dipoleFit/mne_sss_data.cpp

HEADERS +=\
    inverse_global.h \
    IInverseAlgorithm.h \
    dipoleFit/ecd.h \
    dipoleFit/ecd_set.h \
    dipoleFit/mne_types.h \
    dipoleFit/fwd_types.h \
    dipoleFit/fiff_explain.h \
    dipoleFit/analyze_types.h \
    dipoleFit/guess_data.h \
    dipoleFit/dipole_forward.h \
    dipoleFit/dipole_fit_data.h \
    dipoleFit/fwd_eeg_sphere_layer.h \
    dipoleFit/fwd_eeg_sphere_model.h \
    dipoleFit/fwd_eeg_sphere_model_set.h \
    dipoleFit/fwd_coil.h \
    dipoleFit/fwd_coil_set.h \
    minimumNorm/minimumnorm.h \
    rapMusic/rapmusic.h \
    rapMusic/pwlrapmusic.h \
    rapMusic/dipole.h \
    dipoleFit/dipole_fit_settings.h \
    dipoleFit/dipole_fit.h \
    dipoleFit/dipolefit_helpers.h \
    dipoleFit/mne_sss_data.h

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

# Install headers to include directory
header_files.files = ./*.h
header_files.path = $${MNE_INCLUDE_DIR}/inverse

header_files_minimum_norm.files = ./minimumNorm/*.h
header_files_minimum_norm.path = $${MNE_INCLUDE_DIR}/inverse/minimumNorm

header_files_rap_music.files = ./rapMusic/*.h
header_files_rap_music.path = $${MNE_INCLUDE_DIR}/inverse/rapMusic

INSTALLS += header_files
INSTALLS += header_files_minimum_norm
INSTALLS += header_files_rap_music

unix: QMAKE_CXXFLAGS += -isystem $$EIGEN_INCLUDE_DIR

# Deploy Qt Dependencies
win32 {
    isEmpty(TARGET_EXT) {
        TARGET_CUSTOM_EXT = .dll
    } else {
        TARGET_CUSTOM_EXT = $${TARGET_EXT}
    }

    DEPLOY_COMMAND = windeployqt

    DEPLOY_TARGET = $$shell_quote($$shell_path($${MNE_BINARY_DIR}/$${TARGET}$${TARGET_CUSTOM_EXT}))

    #  # Uncomment the following line to help debug the deploy command when running qmake
    #  warning($${DEPLOY_COMMAND} $${DEPLOY_TARGET})
    QMAKE_POST_LINK += $${DEPLOY_COMMAND} $${DEPLOY_TARGET}
}

DISTFILES += \
    dipoleFit/dipolefit_helpers_bak.txt
