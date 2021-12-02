#==============================================================================================================
#
# @file     utils.pro
# @author   Lars Debor <Lars.Debor@tu-ilmenau.de>;
#           Daniel Strohmeier <Daniel.Strohmeier@tu-ilmenau.de>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Christoph Dinh <chdinh@nmr.mgh.harvard.edu>;
#           Juan GPC <jgarciaprieto@mgh.harvard.edu>
# @since    0.1.0
# @date     July, 2012
#
# @section  LICENSE
#
# Copyright (C) 2021, Lars Debor, Daniel Strohmeier, Lorenz Esch, Christoph Dinh, Juan GPC. All rights reserved.
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
# @brief    This project file builds the Utils library.
#
#==============================================================================================================

include(../../mne-cpp.pri)

TEMPLATE = lib

CONFIG += skip_target_version_ext

QT -= gui
QT += concurrent

DEFINES += UTILS_LIBRARY

DESTDIR = $${MNE_LIBRARY_DIR}

TARGET = Utils
TARGET = $$join(TARGET,,"mnecpp",)
CONFIG(debug, debug|release) {
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += staticlib
    DEFINES += STATICBUILD
} else {
    CONFIG += shared
}

SOURCES += \
    file.cpp \
    kmeans.cpp \
    mnemath.cpp \
    ioutils.cpp \
    layoutloader.cpp \
    layoutmaker.cpp \
    selectionio.cpp \
    spectrogram.cpp \
    utils_global.cpp \
    warp.cpp \
    sphere.cpp \
    generics/observerpattern.cpp \
    generics/applicationlogger.cpp \
    spectral.cpp \
    mnetracer.cpp

HEADERS += \
    buildinfo.h \
    file.h \
    kmeans.h\
    utils_global.h \
    mnemath.h \
    ioutils.h \
    layoutloader.h \
    layoutmaker.h \
    selectionio.h \
    spectrogram.h \
    warp.h \
    sphere.h \
    simplex_algorithm.h \
    generics/circularbuffer.h \
    generics/commandpattern.h \
    generics/observerpattern.h \
    generics/applicationlogger.h \
    spectral.h \
    mnetracer.h

clang {
    QMAKE_CXXFLAGS += -isystem $${EIGEN_INCLUDE_DIR} 
} else {
    INCLUDEPATH += $${EIGEN_INCLUDE_DIR} 
}
INCLUDEPATH += $${MNE_INCLUDE_DIR}

contains(MNECPP_CONFIG, withCodeCov) {
    QMAKE_CXXFLAGS += --coverage
    QMAKE_LFLAGS += --coverage
}

win32:!contains(MNECPP_CONFIG, static) {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($${MNE_LIBRARY_DIR}/$${TARGET}.dll) $${MNE_BINARY_DIR}
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

FILE_TO_UPDATE = utils_global.cpp
win32 {
    CONFIG(debug, debug|release) {
        OBJ_TARJET = debug\utils_global.obj
    } else {
        OBJ_TARJET = release\utils_global.obj
    }
}

ALL_FILES += $$HEADERS
ALL_FILES += $$SOURCES
ALL_FILES -= $$FILE_TO_UPDATE

FileUpdater.target = phonyFileUpdater
for (I_FILE, ALL_FILES) {
    FileUpdater.depends += $${PWD}/$${I_FILE}
}

unix|macx {
    FileUpdater.commands = touch $${PWD}/$${FILE_TO_UPDATE} ; echo PASTA > phonyFileUpdater
}

win32 {
    FileUpdater.commands = copy /y $$shell_path($${PWD})\\$${FILE_TO_UPDATE} +,, $$shell_path($${PWD})\\$${FILE_TO_UPDATE} & echo PASTA > phonyFileUpdater
    OrderForcerTarget.target = $${OBJ_TARJET}
    OrderForcerTarget.depends += phonyFileUpdater
    QMAKE_EXTRA_TARGETS += OrderForcerTarget
}

PRE_TARGETDEPS += phonyFileUpdater
QMAKE_EXTRA_TARGETS += FileUpdater

