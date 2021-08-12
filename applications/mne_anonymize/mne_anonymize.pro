#==============================================================================================================
#
# @file     mne_anonymize.pro
# @author   Juan Garcia-Prieto <juangpc@gmail.com>;
#           Lorenz Esch <lesch@mgh.harvard.edu>;
#           Matti Hamalainen <msh@nmr.mgh.harvard.edu>;
#           Wayne Mead <wayne.mead@uth.tmc.edu>;
#           John C. Mosher <John.C.Mosher@uth.tmc.edu>
# @since    0.1.0
# @date     August, 2019
#
# @section  LICENSE
#
# Copyright (C) 2019, Juan Garcia-Prieto, Lorenz Esch, Matti Hamalainen, Wayne Mead, John C. Mosher. All rights reserved.
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
# @brief    This project file generates the makefile to build the mne_anonymize application.
#
#==============================================================================================================

include(../../mne-cpp.pri)

QMAKE_TARGET_DESCRIPTION = MNE Anonymize

#Application version
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_BUILD = 0

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR"\
       "VERSION_MINOR=$$VERSION_MINOR"\
       "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}

TEMPLATE = app

QT += core widgets network

!contains(MNECPP_CONFIG, withAppBundles) {
    CONFIG -= app_bundle
}

DESTDIR = $${MNE_BINARY_DIR}

TARGET = mne_anonymize

CONFIG += console

CONFIG(debug, debug|release) {
#    CONFIG += console
    TARGET = $$join(TARGET,,,d)
}

contains(MNECPP_CONFIG, static) {
    CONFIG += static
    DEFINES += STATICBUILD
}

contains(MNECPP_CONFIG, wasm) {
#    QMAKE_LFLAGS += -s ERROR_ON_UNDEFINED_SYMBOLS=1
#    QMAKE_LFLAGS += -s ASSERTIONS=1
#    QMAKE_LFLAGS += -s STRICT=0
#    QMAKE_LFLAGS += -s FORCE_FILESYSTEM=1
#    QMAKE_LFLAGS += TOTAL_MEMORY=4GB
#    QMAKE_CFLAGS += TOTAL_MEMORY=4GB
#    QMAKE_CXXFLAGS += TOTAL_MEMORY=4GB

    DEFINES += __EMSCRIPTEN__
    LIBS += -lidbfs.js
    INCLUDEPATH += /home/lorenz/Git/emsdk/usptream/emscripten/src
    DEFINES += WASMBUILD
}

LIBS += -L$${MNE_LIBRARY_DIR}
CONFIG(debug, debug|release) {
    LIBS += -lmnecppFiffd \
            -lmnecppUtilsd \
} else {
    LIBS += -lmnecppFiff \
            -lmnecppUtils \
}

SOURCES += \
    main.cpp \
    apphandler.cpp \
    fiffanonymizer.cpp \
    mainwindow.cpp \
    settingscontrollercl.cpp \
    settingscontrollergui.cpp

HEADERS += \
    apphandler.h \
    fiffanonymizer.h \
    mainwindow.h \
    settingscontrollercl.h \
    settingscontrollergui.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += $${EIGEN_INCLUDE_DIR}
INCLUDEPATH += $${MNE_INCLUDE_DIR}
INCLUDEPATH += $${MNE_GLOBAL_DIR}
INCLUDEPATH += $${MNE_FIFF_ANONYMIZER_DIR}

unix:!macx {
    QMAKE_RPATHDIR += $ORIGIN/../lib
}

macx {
    QMAKE_LFLAGS += -Wl,-rpath,@executable_path/../lib
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

