#==============================================================================================================
#
# @file     fwd.pro
# @author   Juan GPC <jgarciaprieto@mgh.harvard.edu>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>
# @since    0.1.0
# @date     January, 2017
#
# @section  LICENSE
#
# Copyright (C) 2017, Lorenz Esch, Matti Hamalainen, Christoph Dinh. All rights reserved.
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
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT += concurrent
QT -= gui

DEFINES += FWD_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Fwd
TARGET = $$join(TARGET,,mnecpp,)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppMned \
            -lmnecppFiffd \
            -lmnecppFsd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppMne \
            -lmnecppFiff \
            -lmnecppFs \
            -lmnecppUtils \
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
    fwd_global.cpp \
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

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    !contains(MNECPP_CONFIG, wasm) {
        QMAKE_CXXFLAGS  +=  -openmp
        #QMAKE_LFLAGS    +=  -openmp
    }

    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
}

unix:!macx:!contains(MNECPP_CONFIG, wasm):!contains(MNECPP_CONFIG, static) {
    QMAKE_CXXFLAGS  +=  -fopenmp
    QMAKE_LFLAGS    +=  -fopenmp
}

macx {
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
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

################################################## BUILD TIMESTAMP/HASH UPDATER ############################################

FILETOUPDATE = fwd_global.cpp

ALLFILES += $$HEADERS
ALLFILES += $$SOURCES
ALLFILES -= $$FILETOUPDATE
FileUpdater.target = phonyFileUpdater
FileUpdater.commands = touch $$PWD/$$FILETOUPDATE ; echo PASTA > phonyFileUpdater
FileUpdater.depends +=
for (IFILE, ALLFILES) {
    FileUpdater.depends += $$PWD/$$IFILE
}
PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater


